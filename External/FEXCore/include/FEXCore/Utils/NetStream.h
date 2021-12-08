#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include <iostream>

namespace FEXCore::Utils {
class FEX_DEFAULT_VISIBILITY NetStream : public std::iostream {
public:
    explicit NetStream(int socketfd);
    ~NetStream() override;
};
} // namespace FEXCore::Utils
