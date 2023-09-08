#include "analysis.h"
#include "diagnostics.h"

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>

#include <fmt/format.h>

struct NamespaceAnnotations {
    std::optional<unsigned> version;
    std::optional<std::string> load_host_endpoint_via;
    bool generate_guest_symtable = false;
    bool indirect_guest_calls = false;
};

static NamespaceAnnotations GetNamespaceAnnotations(clang::ASTContext& context, clang::CXXRecordDecl* decl) {
    if (!decl->hasDefinition()) {
        return {};
    }

    ErrorReporter report_error { context };
    NamespaceAnnotations ret;

    for (const clang::CXXBaseSpecifier& base : decl->bases()) {
        auto annotation = base.getType().getAsString();
        if (annotation == "fexgen::generate_guest_symtable") {
            ret.generate_guest_symtable = true;
        } else if (annotation == "fexgen::indirect_guest_calls") {
            ret.indirect_guest_calls = true;
        } else {
            throw report_error(base.getSourceRange().getBegin(), "Unknown namespace annotation");
        }
    }

    for (const clang::FieldDecl* field : decl->fields()) {
        auto name = field->getNameAsString();
        if (name == "load_host_endpoint_via") {
            auto loader_function_expr = field->getInClassInitializer()->IgnoreCasts();
            auto loader_function_str = llvm::dyn_cast_or_null<clang::StringLiteral>(loader_function_expr);
            if (loader_function_expr && !loader_function_str) {
                throw report_error(loader_function_expr->getBeginLoc(),
                                          "Must initialize load_host_endpoint_via with a string");
            }
            if (loader_function_str) {
                ret.load_host_endpoint_via = loader_function_str->getString();
            }
        } else if (name == "version") {
            auto initializer = field->getInClassInitializer()->IgnoreCasts();
            auto version_literal = llvm::dyn_cast_or_null<clang::IntegerLiteral>(initializer);
            if (!initializer || !version_literal) {
                throw report_error(field->getBeginLoc(), "No version given (expected integral typed member, e.g. \"int version = 5;\")");
            }
            ret.version = version_literal->getValue().getZExtValue();
        } else {
            throw report_error(field->getBeginLoc(), "Unknown namespace annotation");
        }
    }

    return ret;
}

enum class CallbackStrategy {
    Default,
    Stub,
    Guest,
};

struct Annotations {
    bool custom_host_impl = false;
    bool custom_guest_entrypoint = false;

    bool returns_guest_pointer = false;

    std::optional<clang::QualType> uniform_va_type;

    CallbackStrategy callback_strategy = CallbackStrategy::Default;
};

static Annotations GetAnnotations(clang::ASTContext& context, clang::CXXRecordDecl* decl) {
    ErrorReporter report_error { context };
    Annotations ret;

    for (const auto& base : decl->bases()) {
        auto annotation = base.getType().getAsString();
        if (annotation == "fexgen::returns_guest_pointer") {
            ret.returns_guest_pointer = true;
        } else if (annotation == "fexgen::custom_host_impl") {
            ret.custom_host_impl = true;
        } else if (annotation == "fexgen::callback_stub") {
            ret.callback_strategy = CallbackStrategy::Stub;
        } else if (annotation == "fexgen::callback_guest") {
            ret.callback_strategy = CallbackStrategy::Guest;
        } else if (annotation == "fexgen::custom_guest_entrypoint") {
            ret.custom_guest_entrypoint = true;
        } else {
            throw report_error(base.getSourceRange().getBegin(), "Unknown annotation");
        }
    }

    for (const auto& child_decl : decl->getPrimaryContext()->decls()) {
        if (auto field = llvm::dyn_cast_or_null<clang::FieldDecl>(child_decl)) {
            throw report_error(field->getBeginLoc(), "Unknown field annotation");
        } else if (auto type_alias = llvm::dyn_cast_or_null<clang::TypedefNameDecl>(child_decl)) {
            auto name = type_alias->getNameAsString();
            if (name == "uniform_va_type") {
                ret.uniform_va_type = type_alias->getUnderlyingType();
            } else {
                throw report_error(type_alias->getBeginLoc(), "Unknown type alias annotation");
            }
        }
    }

    return ret;
}

void AnalysisAction::ExecuteAction() {
    clang::ASTFrontendAction::ExecuteAction();

    // Post-processing happens here rather than in an overridden EndSourceFileAction implementation.
    // We can't move the logic to the latter since this code might still raise errors, but
    // clang's diagnostics engine is already shut down by the time EndSourceFileAction is called.

    auto& context = getCompilerInstance().getASTContext();
    if (context.getDiagnostics().hasErrorOccurred()) {
        return;
    }
    decl_contexts.front() = context.getTranslationUnitDecl();

    try {
        ParseInterface(context);
        EmitOutput(context);
    } catch (ClangDiagnosticAsException& exception) {
        exception.Report(context.getDiagnostics());
    }
}

static clang::ClassTemplateDecl*
FindClassTemplateDeclByName(clang::DeclContext& decl_context, std::string_view symbol_name) {
    auto& ast_context = decl_context.getParentASTContext();
    auto* ident = &ast_context.Idents.get(symbol_name);
    auto declname = ast_context.DeclarationNames.getIdentifier(ident);
    auto result = decl_context.noload_lookup(declname);
    if (result.empty()) {
        return nullptr;
    } else if (std::next(result.begin()) == result.end()) {
        return llvm::dyn_cast<clang::ClassTemplateDecl>(*result.begin());
    } else {
        throw std::runtime_error("Found multiple matches to symbol " + std::string { symbol_name });
    }
}

void AnalysisAction::ParseInterface(clang::ASTContext& context) {
    ErrorReporter report_error { context };

    if (auto template_decl = FindClassTemplateDeclByName(*context.getTranslationUnitDecl(), "fex_gen_type")) {
        for (auto* decl : template_decl->specializations()) {
            const auto& template_args = decl->getTemplateArgs();
            assert(template_args.size() == 1);

            // NOTE: Function types that are equivalent but use differently
            //       named types (e.g. GLuint/GLenum) are represented by
            //       different Type instances. The canonical type they refer
            //       to is unique, however.
            auto type = context.getCanonicalType(template_args[0].getAsType()).getTypePtr();
            funcptr_types.insert(type);
        }
    }

    // Process declarations and specializations of fex_gen_config,
    // i.e. the function descriptions of the thunked API
    for (auto& decl_context : decl_contexts) {
        if (const auto template_decl = FindClassTemplateDeclByName(*decl_context, "fex_gen_config")) {
            // Gather general information about symbols in this namespace
            const auto annotations = GetNamespaceAnnotations(context, template_decl->getTemplatedDecl());

            auto namespace_decl = llvm::dyn_cast<clang::NamespaceDecl>(decl_context);
            namespaces.push_back({  namespace_decl,
                                    namespace_decl ? namespace_decl->getNameAsString() : "",
                                    annotations.load_host_endpoint_via.value_or(""),
                                    annotations.generate_guest_symtable,
                                    annotations.indirect_guest_calls });
            const auto namespace_idx = namespaces.size() - 1;
            const NamespaceInfo& namespace_info = namespaces.back();

            if (annotations.version) {
                if (namespace_decl) {
                    throw report_error(template_decl->getBeginLoc(), "Library version must be defined in the global namespace");
                }
                lib_version = annotations.version;
            }

            // Process specializations of template fex_gen_config
            // First, perform some validation and process member annotations
            // In a second iteration, process the actual function API
            for (auto* decl : template_decl->specializations()) {
                if (decl->getSpecializationKind() == clang::TSK_ExplicitInstantiationDefinition) {
                    throw report_error(decl->getBeginLoc(), "fex_gen_config may not be partially specialized\n");
                }

                const auto& template_args = decl->getTemplateArgs();
                assert(template_args.size() == 1);

                const auto template_arg_loc = decl->getTypeAsWritten()->getTypeLoc().castAs<clang::TemplateSpecializationTypeLoc>().getArgLoc(0).getLocation();

                if (auto emitted_function = llvm::dyn_cast<clang::FunctionDecl>(template_args[0].getAsDecl())) {
                    // Process later
                } else {
                    throw report_error(template_arg_loc, "Cannot annotate this kind of symbol");
                }
            }

            // Process API functions
            for (auto* decl : template_decl->specializations()) {
                if (decl->getSpecializationKind() == clang::TSK_ExplicitInstantiationDefinition) {
                    throw report_error(decl->getBeginLoc(), "fex_gen_config may not be partially specialized\n");
                }

                const auto& template_args = decl->getTemplateArgs();
                assert(template_args.size() == 1);

                const auto template_arg_loc = decl->getTypeAsWritten()->getTypeLoc().castAs<clang::TemplateSpecializationTypeLoc>().getArgLoc(0).getLocation();

                auto emitted_function = llvm::dyn_cast<clang::FunctionDecl>(template_args[0].getAsDecl());
                assert(emitted_function && "Argument is not a function");
                auto return_type = emitted_function->getReturnType();

                const auto annotations = GetAnnotations(context, decl);
                if (return_type->isFunctionPointerType() && !annotations.returns_guest_pointer) {
                    throw report_error( template_arg_loc,
                                        "Function pointer return types require explicit annotation\n");
                }

                // TODO: Use the types as written in the signature instead?
                ThunkedFunction data;
                data.function_name = emitted_function->getName().str();
                data.return_type = return_type;
                data.is_variadic = emitted_function->isVariadic();

                data.decl = emitted_function;

                data.custom_host_impl = annotations.custom_host_impl;

                for (std::size_t param_idx = 0; param_idx < emitted_function->param_size(); ++param_idx) {
                    auto* param = emitted_function->getParamDecl(param_idx);
                    data.param_types.push_back(param->getType());

                    if (param->getType()->isFunctionPointerType()) {
                        auto funcptr = param->getFunctionType()->getAs<clang::FunctionProtoType>();
                        ThunkedCallback callback;
                        callback.return_type = funcptr->getReturnType();
                        for (auto& cb_param : funcptr->getParamTypes()) {
                            callback.param_types.push_back(cb_param);
                        }
                        callback.is_stub = annotations.callback_strategy == CallbackStrategy::Stub;
                        callback.is_guest = annotations.callback_strategy == CallbackStrategy::Guest;
                        callback.is_variadic = funcptr->isVariadic();

                        if (callback.is_guest && !data.custom_host_impl) {
                            throw report_error(template_arg_loc, "callback_guest can only be used with custom_host_impl");
                        }

                        data.callbacks.emplace(param_idx, callback);
                        if (!callback.is_stub && !callback.is_guest) {
                            funcptr_types.insert(context.getCanonicalType(funcptr));
                        }

                        if (data.callbacks.size() != 1) {
                            throw report_error(template_arg_loc, "Support for more than one callback is untested");
                        }
                        if (funcptr->isVariadic() && !callback.is_stub) {
                            throw report_error(template_arg_loc, "Variadic callbacks are not supported");
                        }
                    }
                }

                thunked_api.push_back(ThunkedAPIFunction { (const FunctionParams&)data, data.function_name, data.return_type,
                                                            namespace_info.host_loader.empty() ? "dlsym_default" : namespace_info.host_loader,
                                                            data.is_variadic || annotations.custom_guest_entrypoint,
                                                            data.is_variadic,
                                                            std::nullopt });
                if (namespace_info.generate_guest_symtable) {
                    thunked_api.back().symtable_namespace = namespace_idx;
                }

                if (data.is_variadic) {
                    if (!annotations.uniform_va_type) {
                        throw report_error(decl->getBeginLoc(), "Variadic functions must be annotated with parameter type using uniform_va_type");
                    }

                    // Convert variadic argument list into a count + pointer pair
                    data.param_types.push_back(context.getSizeType());
                    data.param_types.push_back(context.getPointerType(*annotations.uniform_va_type));
                }

                if (data.is_variadic) {
                    // This function is thunked through an "_internal" symbol since its signature
                    // is different from the one in the native host/guest libraries.
                    data.function_name = data.function_name + "_internal";
                    if (data.custom_host_impl) {
                        throw report_error(decl->getBeginLoc(), "Custom host impl requested but this is implied by the function signature already");
                    }
                    data.custom_host_impl = true;
                }

                // For indirect calls, register the function signature as a function pointer type
                if (namespace_info.indirect_guest_calls) {
                    funcptr_types.insert(context.getCanonicalType(emitted_function->getFunctionType()));
                }

                thunks.push_back(std::move(data));
            }
        }
    }
}

class ASTVisitor : public clang::RecursiveASTVisitor<ASTVisitor> {
    std::vector<clang::DeclContext*>& decl_contexts;

public:
    ASTVisitor(std::vector<clang::DeclContext*>& decl_contexts_)
        : decl_contexts(decl_contexts_) {
    }

    /**
     * Matches "template<auto> struct fex_gen_config { ... }"
     */
    bool VisitClassTemplateDecl(clang::ClassTemplateDecl* decl) {
        if (decl->getName() != "fex_gen_config") {
            return true;
        }

        if (llvm::dyn_cast<clang::NamespaceDecl>(decl->getDeclContext())) {
            decl_contexts.push_back(decl->getDeclContext());
        }

        return true;
    }
};

class ASTConsumer : public clang::ASTConsumer {
    std::vector<clang::DeclContext*>& decl_contexts;

public:
    ASTConsumer(std::vector<clang::DeclContext*>& decl_contexts_)
        : decl_contexts(decl_contexts_) {
    }

    void HandleTranslationUnit(clang::ASTContext& context) override {
        ASTVisitor { decl_contexts }.TraverseDecl(context.getTranslationUnitDecl());
    }
};

std::unique_ptr<clang::ASTConsumer> AnalysisAction::CreateASTConsumer(clang::CompilerInstance&, clang::StringRef) {
    return std::make_unique<ASTConsumer>(decl_contexts);
}
