#include <clang/Tooling/Tooling.h>

#include <optional>
#include <string>

struct OutputFilenames {
    std::string host;
    std::string guest;
};

class GenerateThunkLibsActionFactory : public clang::tooling::FrontendActionFactory {
public:
    GenerateThunkLibsActionFactory(std::string_view libname_, OutputFilenames output_filenames_)
        : libname(std::move(libname_)), output_filenames(std::move(output_filenames_)) {
    }

    std::unique_ptr<clang::FrontendAction> create() override;

private:
    std::string libname;
    OutputFilenames output_filenames;
};
