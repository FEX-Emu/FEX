#pragma once

namespace FEXCore::CPU {

static constexpr unsigned STATE_arm64 = 28; // r28
static constexpr unsigned STATE_x86 = 14; // r14

#ifdef _M_ARM_64
static constexpr unsigned STATE_host = STATE_arm64;
#endif
#ifdef _M_X86_64
static constexpr unsigned STATE_host = STATE_x86;
#endif

}
