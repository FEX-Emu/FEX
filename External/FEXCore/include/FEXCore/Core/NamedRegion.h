#pragma once
#include <memory>
#include <string>


namespace FEXCore::HLE {
    struct SourcecodeMap;
}

namespace FEXCore {
    class CodeCache;
}

namespace FEXCore::Core {

struct NamedRegion {
    std::string FileId;
    std::string Filename;
    std::string Fingerprint;
    
    std::unique_ptr<FEXCore::CodeCache> ObjCache;
    std::unique_ptr<FEXCore::CodeCache> IRCache;
    std::unique_ptr<FEXCore::HLE::SourcecodeMap> SourcecodeMap;
    
    bool ContainsCode;
};

}