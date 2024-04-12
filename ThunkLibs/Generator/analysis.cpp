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
        CoverReferencedTypes(context);
        OnAnalysisComplete(context);
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

struct TypeAnnotations {
    bool is_opaque = false;
    bool assumed_compatible = false;
    bool emit_layout_wrappers = false;
};

static TypeAnnotations GetTypeAnnotations(clang::ASTContext& context, clang::CXXRecordDecl* decl) {
    if (!decl->hasDefinition()) {
        return {};
    }

    ErrorReporter report_error { context };
    TypeAnnotations ret;

    for (const clang::CXXBaseSpecifier& base : decl->bases()) {
        auto annotation = base.getType().getAsString();
        if (annotation == "fexgen::opaque_type") {
            ret.is_opaque = true;
        } else if (annotation == "fexgen::assume_compatible_data_layout") {
            ret.assumed_compatible = true;
        } else if (annotation == "fexgen::emit_layout_wrappers") {
            ret.emit_layout_wrappers = true;
        } else {
            throw report_error(base.getSourceRange().getBegin(), "Unknown type annotation");
        }
    }

    return ret;
}

static ParameterAnnotations GetParameterAnnotations(clang::ASTContext& context, clang::CXXRecordDecl* decl) {
    if (!decl->hasDefinition()) {
        return {};
    }

    ErrorReporter report_error { context };
    ParameterAnnotations ret;

    for (const clang::CXXBaseSpecifier& base : decl->bases()) {
        auto annotation = base.getType().getAsString();
        if (annotation == "fexgen::ptr_passthrough") {
            ret.is_passthrough = true;
        } else if (annotation == "fexgen::assume_compatible_data_layout") {
            ret.assume_compatible = true;
        } else {
            throw report_error(base.getSourceRange().getBegin(), "Unknown parameter annotation");
        }
    }

    return ret;
}

void AnalysisAction::ParseInterface(clang::ASTContext& context) {
    ErrorReporter report_error { context };

    const std::unordered_map<unsigned, ParameterAnnotations> no_param_annotations {};

    // TODO: Assert fex_gen_type is not declared at non-global namespaces
    if (auto template_decl = FindClassTemplateDeclByName(*context.getTranslationUnitDecl(), "fex_gen_type")) {
        for (auto* decl : template_decl->specializations()) {
            const auto& template_args = decl->getTemplateArgs();
            assert(template_args.size() == 1);

            // NOTE: Function types that are equivalent but use differently
            //       named types (e.g. GLuint/GLenum) are represented by
            //       different Type instances. The canonical type they refer
            //       to is unique, however.
            clang::QualType type = context.getCanonicalType(template_args[0].getAsType());
            type = type->getLocallyUnqualifiedSingleStepDesugaredType();

            const auto annotations = GetTypeAnnotations(context, decl);
            if (type->isFunctionPointerType() || type->isFunctionType()) {
                if (decl->getNumBases()) {
                    throw report_error(decl->getBeginLoc(), "Function pointer types cannot be annotated");
                }
                thunked_funcptrs[type.getAsString()] = std::pair { type.getTypePtr(), no_param_annotations };
            } else {
                RepackedType repack_info = {
                    .assumed_compatible = annotations.is_opaque || annotations.assumed_compatible,
                    .pointers_only = annotations.is_opaque && !annotations.assumed_compatible,
                    .emit_layout_wrappers = annotations.emit_layout_wrappers
                };
                [[maybe_unused]] auto [it, inserted] = types.emplace(context.getCanonicalType(type.getTypePtr()), repack_info);
                assert(inserted);
            }
        }
    }

    // Process function parameter annotations
    std::unordered_map<const clang::FunctionDecl*, std::unordered_map<unsigned, ParameterAnnotations>> param_annotations;
    for (auto& decl_context : decl_contexts) {
        if (auto template_decl = FindClassTemplateDeclByName(*decl_context, "fex_gen_param")) {
            for (auto* decl : template_decl->specializations()) {
                const auto& template_args = decl->getTemplateArgs();
                assert(template_args.size() == 3);

                auto function = llvm::dyn_cast<clang::FunctionDecl>(template_args[0].getAsDecl());
                auto param_idx = template_args[1].getAsIntegral().getZExtValue();
                clang::QualType type = context.getCanonicalType(template_args[2].getAsType());
                type = type->getLocallyUnqualifiedSingleStepDesugaredType();

                if (param_idx >= function->getNumParams()) {
                    throw report_error(decl->getTypeAsWritten()->getTypeLoc().getAs<clang::TemplateSpecializationTypeLoc>().getArgLoc(1).getLocation(), "Out-of-bounds parameter index passed to fex_gen_param");
                }

                if (!type->isVoidType() && !context.hasSameType(type, function->getParamDecl(param_idx)->getType())) {
                    throw report_error(decl->getTypeAsWritten()->getTypeLoc().getAs<clang::TemplateSpecializationTypeLoc>().getArgLoc(2).getLocation(), "Type passed to fex_gen_param doesn't match the function signature")
                              .addNote(report_error(function->getParamDecl(param_idx)->getTypeSourceInfo()->getTypeLoc().getBeginLoc(), "Expected this type instead"));
                }

                param_annotations[function][param_idx] = GetParameterAnnotations(context, decl);
            }
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
                } else if (auto annotated_member = llvm::dyn_cast<clang::FieldDecl>(template_args[0].getAsDecl())) {
                    if (decl->getNumBases() != 1 || decl->bases_begin()->getType().getAsString() != "fexgen::custom_repack") {
                        throw report_error(template_arg_loc, "Unsupported member annotation(s)");
                    }

                    // Get or add parent type to list of structure types
                    auto repack_info_it = types.emplace(context.getCanonicalType(annotated_member->getParent()->getTypeForDecl()), RepackedType {}).first;
                    if (repack_info_it->second.assumed_compatible) {
                        throw report_error(template_arg_loc, "May not annotate members of opaque types");
                    }
                    // Add member to its list of members
                    repack_info_it->second.custom_repacked_members.insert(annotated_member->getNameAsString());
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

                if (auto emitted_function = llvm::dyn_cast<clang::FunctionDecl>(template_args[0].getAsDecl())) {
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

                    data.param_annotations = param_annotations[emitted_function];

                    const int retval_index = -1;
                    for (int param_idx = retval_index; param_idx < (int)emitted_function->param_size(); ++param_idx) {
                        auto param_type = param_idx == retval_index ? emitted_function->getReturnType() : emitted_function->getParamDecl(param_idx)->getType();
                        auto param_loc = param_idx == retval_index ? emitted_function->getReturnTypeSourceRange().getBegin() : emitted_function->getParamDecl(param_idx)->getBeginLoc();

                        if (param_idx != retval_index) {
                            data.param_types.push_back(param_type);
                        } else if (param_type->isVoidType()) {
                            continue;
                        }

                        // Skip pointers-to-structs passed through to the host in guest_layout.
                        // This avoids pulling in member types that can't be processed.
                        if (data.param_annotations[param_idx].is_passthrough &&
                            param_type->isPointerType() && param_type->getPointeeType()->isStructureType()) {
                            continue;
                        }

                        auto check_struct_type = [&](const clang::Type* type) {
                            if (type->isIncompleteType()) {
                                throw report_error(type->getAsTagDecl()->getBeginLoc(), "Unannotated pointer with incomplete struct type; consider using an opaque_type annotation")
                                      .addNote(report_error(emitted_function->getNameInfo().getLoc(), "in function", clang::DiagnosticsEngine::Note))
                                      .addNote(report_error(template_arg_loc, "used in annotation here", clang::DiagnosticsEngine::Note));
                            }

                            for (auto* member : type->getAsStructureType()->getDecl()->fields()) {
                                auto annotated_type = types.find(type->getCanonicalTypeUnqualified().getTypePtr());
                                if (annotated_type == types.end() || !annotated_type->second.UsesCustomRepackFor(member)) {
                                    /*if (!member->getType()->isPointerType())*/ {
                                        // TODO: Perform more elaborate validation for non-pointers to ensure ABI compatibility
                                        continue;
                                    }

                                    throw report_error(member->getBeginLoc(), "Unannotated pointer member")
                                          .addNote(report_error(param_loc, "in struct type", clang::DiagnosticsEngine::Note))
                                          .addNote(report_error(template_arg_loc, "used in annotation here", clang::DiagnosticsEngine::Note));
                                }
                            }
                        };

                        if (param_type->isFunctionPointerType()) {
                            if (param_idx == retval_index) {
                                // TODO: We already rely on this in a few places...
//                                throw report_error(template_arg_loc, "Support for returning function pointers is not implemented");
                                continue;
                            }
                            auto funcptr = emitted_function->getParamDecl(param_idx)->getFunctionType()->getAs<clang::FunctionProtoType>();
                            ThunkedCallback callback;
                            callback.return_type = funcptr->getReturnType();
                            for (auto& cb_param : funcptr->getParamTypes()) {
                                callback.param_types.push_back(cb_param);
                            }
                            callback.is_stub = annotations.callback_strategy == CallbackStrategy::Stub;
                            callback.is_variadic = funcptr->isVariadic();

                            data.callbacks.emplace(param_idx, callback);
                            if (!callback.is_stub && !data.custom_host_impl) {
                                thunked_funcptrs[emitted_function->getNameAsString() + "_cb" + std::to_string(param_idx)] = std::pair { context.getCanonicalType(funcptr), no_param_annotations };
                            }

                            if (data.callbacks.size() != 1) {
                                throw report_error(template_arg_loc, "Support for more than one callback is untested");
                            }
                            if (funcptr->isVariadic() && !callback.is_stub) {
                                throw report_error(template_arg_loc, "Variadic callbacks are not supported");
                            }

                            // Force treatment as passthrough-pointer
                            data.param_annotations[param_idx].is_passthrough = true;
                        } else if (param_type->isBuiltinType()) {
                            // NOTE: Intentionally not using getCanonicalType here since that would turn e.g. size_t into platform-specific types
                            // TODO: Still, we may want to de-duplicate some of these...
                            types.emplace(param_type.getTypePtr(), RepackedType { });
                        } else if (param_type->isEnumeralType()) {
                            types.emplace(context.getCanonicalType(param_type.getTypePtr()), RepackedType { });
                        } else if ( param_type->isStructureType() &&
                                    !(types.contains(context.getCanonicalType(param_type.getTypePtr())) &&
                                      LookupType(context, param_type.getTypePtr()).assumed_compatible)) {
                            check_struct_type(param_type.getTypePtr());
                            types.emplace(context.getCanonicalType(param_type.getTypePtr()), RepackedType { });
                        } else if (param_type->isPointerType()) {
                            auto pointee_type = param_type->getPointeeType();

                            if (pointee_type->isIntegerType()) {
                                // Add builtin pointee type to type list
                                if (!pointee_type->isEnumeralType()) {
                                    types.emplace(pointee_type.getTypePtr(), RepackedType { });
                                } else {
                                    types.emplace(context.getCanonicalType(pointee_type.getTypePtr()), RepackedType { });
                                }
                            }

                            if (data.param_annotations[param_idx].assume_compatible) {
                                // Nothing to do
                            } else if (types.contains(context.getCanonicalType(pointee_type.getTypePtr())) && LookupType(context, pointee_type.getTypePtr()).assumed_compatible) {
                                // Parameter points to a type that is assumed compatible
                                data.param_annotations[param_idx].assume_compatible = true;
                            } else if (pointee_type->isStructureType()) {
                                // Unannotated pointer to unannotated structure.
                                // Append the structure type to the type list for checking data layout compatibility.
                                check_struct_type(pointee_type.getTypePtr());
                                types.emplace(context.getCanonicalType(pointee_type.getTypePtr()), RepackedType { });
                            } else if (data.param_annotations[param_idx].is_passthrough) {
                                if (!data.custom_host_impl) {
                                    throw report_error(param_loc, "Passthrough annotation requires custom host implementation");
                                }

                                // Nothing to do
                            } else if (false /* TODO: Can't check if this is unsupported until data layout analysis is complete */) {
                                throw report_error(param_loc, "Unsupported parameter type")
                                              .addNote(report_error(emitted_function->getNameInfo().getLoc(), "in function", clang::DiagnosticsEngine::Note))
                                              .addNote(report_error(template_arg_loc, "used in definition here", clang::DiagnosticsEngine::Note));
                            }
                        } else {
                            // TODO: For non-pointer parameters, perform more elaborate validation to ensure ABI compatibility
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
                        types.emplace(context.getSizeType()->getTypePtr(), RepackedType { });
                        if (!annotations.uniform_va_type.value()->isVoidPointerType()) {
                            types.emplace(annotations.uniform_va_type->getTypePtr(), RepackedType { });
                        }
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
                        thunked_funcptrs[emitted_function->getNameAsString()] = std::pair { context.getCanonicalType(emitted_function->getFunctionType()), data.param_annotations };
                    }

                    thunks.push_back(std::move(data));
                }
            }
        }
    }
}

void AnalysisAction::CoverReferencedTypes(clang::ASTContext& context) {
    // Add common fixed-size integer types explicitly
    for (unsigned size : { 8, 32, 64 }) {
        types.emplace(context.getIntTypeForBitwidth(size, false).getTypePtr(), RepackedType {});
        types.emplace(context.getIntTypeForBitwidth(size, true).getTypePtr(), RepackedType {});
    }

    // Repeat until no more children are appended
    for (bool changed = true; std::exchange(changed, false);) {
        for ( auto next_type_it = types.begin(), type_it = next_type_it;
              type_it != types.end();
              type_it = next_type_it) {
            ++next_type_it;
            const auto& [type, type_repack_info] = *type_it;
            if (!type->isStructureType()) {
                continue;
            }

            if (type_repack_info.assumed_compatible) {
                // If assumed compatible, we don't need the member definitions
                continue;
            }

            for (auto* member : type->getAsStructureType()->getDecl()->fields()) {
                auto member_type = member->getType().getTypePtr();
                while (member_type->isArrayType()) {
                    member_type = member_type->getArrayElementTypeNoTypeQual();
                }
                while (member_type->isPointerType()) {
                    member_type = member_type->getPointeeType().getTypePtr();
                }

                if (!member_type->isBuiltinType()) {
                    member_type = context.getCanonicalType(member_type);
                }
                if (types.contains(member_type) && types.at(member_type).pointers_only) {
                    if (member_type == context.getCanonicalType(member->getType().getTypePtr())) {
                        throw std::runtime_error(fmt::format("\"{}\" references opaque type \"{}\" via non-pointer member \"{}\"",
                                                             clang::QualType { type, 0 }.getAsString(),
                                                             clang::QualType { member_type, 0 }.getAsString(),
                                                             member->getNameAsString()));
                    }
                    continue;
                }
                if (member_type->isUnionType() && !types.contains(member_type) && !type_repack_info.UsesCustomRepackFor(member)) {
                    throw std::runtime_error(fmt::format("\"{}\" has unannotated member \"{}\" of union type \"{}\"",
                                                         clang::QualType { type, 0 }.getAsString(),
                                                         member->getNameAsString(),
                                                         clang::QualType { member_type, 0 }.getAsString()));
                }

                if (!member_type->isStructureType() && !(member_type->isBuiltinType() && !member_type->isVoidType()) && !member_type->isEnumeralType()) {
                    continue;
                }

                auto [new_type_it, inserted] = types.emplace(member_type, RepackedType { });
                if (inserted) {
                    changed = true;
                    next_type_it = new_type_it;
                }
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
