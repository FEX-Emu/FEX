#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>

#include <utility>
#include <vector>

struct ClangDiagnosticAsException {
    std::pair<clang::SourceLocation, unsigned> diagnostic;

    std::vector<ClangDiagnosticAsException> notes;

    // List of callbacks that add an argument to a clang::DiagnosticBuilder
    std::vector<std::function<void(clang::DiagnosticBuilder&)>> args;

    ClangDiagnosticAsException& AddString(std::string str) {
        args.push_back([arg=std::move(str)](clang::DiagnosticBuilder& db) {
            db.AddString(arg);
        });
        return *this;
    }

    ClangDiagnosticAsException& AddTaggedVal(clang::QualType type) {
        args.push_back([val=type](clang::DiagnosticBuilder& db) {
            db.AddTaggedVal(reinterpret_cast<uintptr_t>(val.getAsOpaquePtr()), clang::DiagnosticsEngine::ak_qualtype);
        });
        return *this;
    }

    ClangDiagnosticAsException& addNote(ClangDiagnosticAsException diagnostic) {
        notes.push_back(std::move(diagnostic));
        return *this;
    }

    void Report(clang::DiagnosticsEngine& diagnostics) const {
        {
            auto builder = diagnostics.Report(diagnostic.first, diagnostic.second);
            for (auto& arg_appender : args) {
                arg_appender(builder);
            }
        }
        for (auto& note : notes) {
            note.Report(diagnostics);
        }
    }
};

// Helper class to build a custom DiagID from the given message and store it in a throwable object
struct ErrorReporter {
    clang::ASTContext& context;

    template<std::size_t N>
    [[nodiscard]] ClangDiagnosticAsException operator()(clang::SourceLocation loc, const char (&message)[N],
                                                        clang::DiagnosticsEngine::Level level = clang::DiagnosticsEngine::Error) {
        auto id = context.getDiagnostics().getCustomDiagID(level, message);
        return { std::pair(loc, id) };
    }
};
