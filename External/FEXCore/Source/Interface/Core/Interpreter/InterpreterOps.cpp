#include "Common/MathUtils.h"
#include "Common/SoftFloat.h"
#include "Interface/Context/Context.h"
#include "InterpreterOps.h"

#ifdef _M_ARM_64
#include "Interface/Core/ArchHelpers/Arm64.h"
#endif
#include "Interface/Core/LookupCache.h"
#include "Interface/Core/DebugData.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/Core/Interpreter/InterpreterClass.h"
#include <FEXCore/Utils/LogManager.h>

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>

#include "Interface/HLE/Thunks/Thunks.h"

#include <atomic>
#include <cmath>
#include <limits>
#include <vector>
#ifdef _M_X86_64
#include <xmmintrin.h>
#endif
#include <unistd.h>

namespace FEXCore::CPU {

namespace AES {
  static __uint128_t InvShiftRows(uint8_t *State) {
    uint8_t Shifted[16] = {
      State[0], State[13], State[10], State[7],
      State[4], State[1], State[14], State[11],
      State[8], State[5], State[2], State[15],
      State[12], State[9], State[6], State[3],
    };
    __uint128_t Res{};
    memcpy(&Res, Shifted, 16);
    return Res;
  }

  static __uint128_t InvSubBytes(uint8_t *State) {
    // 16x16 matrix table
    static const uint8_t InvSubstitutionTable[256] = {
      0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
      0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
      0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
      0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
      0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
      0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
      0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
      0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
      0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
      0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
      0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
      0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
      0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
      0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
      0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
      0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d,
    };

    // Uses a byte substitution table with a constant set of values
    // Needs to do a table look up
    uint8_t Substituted[16];
    for (size_t i = 0; i < 16; ++i) {
      Substituted[i] = InvSubstitutionTable[State[i]];
    }

    __uint128_t Res{};
    memcpy(&Res, Substituted, 16);
    return Res;
  }

  static __uint128_t ShiftRows(uint8_t *State) {
    uint8_t Shifted[16] = {
      State[0], State[5], State[10], State[15],
      State[4], State[9], State[14], State[3],
      State[8], State[13], State[2], State[7],
      State[12], State[1], State[6], State[11],
    };
    __uint128_t Res{};
    memcpy(&Res, Shifted, 16);
    return Res;
  }

  static __uint128_t SubBytes(uint8_t *State, size_t Bytes) {
    // 16x16 matrix table
    static const uint8_t SubstitutionTable[256] = {
      0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
      0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
      0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
      0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
      0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
      0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
      0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
      0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
      0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
      0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
      0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
      0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
      0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
      0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
      0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
      0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16,
    };
    // Uses a byte substitution table with a constant set of values
    // Needs to do a table look up
    uint8_t Substituted[16];
    Bytes = std::min(Bytes, (size_t)16);
    for (size_t i = 0; i < Bytes; ++i) {
      Substituted[i] = SubstitutionTable[State[i]];
    }

    __uint128_t Res{};
    memcpy(&Res, Substituted, Bytes);
    return Res;
  }

  static uint8_t FFMul02(uint8_t in) {
    static const uint8_t FFMul02[256] = {
      0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e,
      0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e,
      0x40, 0x42, 0x44, 0x46, 0x48, 0x4a, 0x4c, 0x4e, 0x50, 0x52, 0x54, 0x56, 0x58, 0x5a, 0x5c, 0x5e,
      0x60, 0x62, 0x64, 0x66, 0x68, 0x6a, 0x6c, 0x6e, 0x70, 0x72, 0x74, 0x76, 0x78, 0x7a, 0x7c, 0x7e,
      0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c, 0x8e, 0x90, 0x92, 0x94, 0x96, 0x98, 0x9a, 0x9c, 0x9e,
      0xa0, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae, 0xb0, 0xb2, 0xb4, 0xb6, 0xb8, 0xba, 0xbc, 0xbe,
      0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce, 0xd0, 0xd2, 0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde,
      0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xec, 0xee, 0xf0, 0xf2, 0xf4, 0xf6, 0xf8, 0xfa, 0xfc, 0xfe,
      0x1b, 0x19, 0x1f, 0x1d, 0x13, 0x11, 0x17, 0x15, 0x0b, 0x09, 0x0f, 0x0d, 0x03, 0x01, 0x07, 0x05,
      0x3b, 0x39, 0x3f, 0x3d, 0x33, 0x31, 0x37, 0x35, 0x2b, 0x29, 0x2f, 0x2d, 0x23, 0x21, 0x27, 0x25,
      0x5b, 0x59, 0x5f, 0x5d, 0x53, 0x51, 0x57, 0x55, 0x4b, 0x49, 0x4f, 0x4d, 0x43, 0x41, 0x47, 0x45,
      0x7b, 0x79, 0x7f, 0x7d, 0x73, 0x71, 0x77, 0x75, 0x6b, 0x69, 0x6f, 0x6d, 0x63, 0x61, 0x67, 0x65,
      0x9b, 0x99, 0x9f, 0x9d, 0x93, 0x91, 0x97, 0x95, 0x8b, 0x89, 0x8f, 0x8d, 0x83, 0x81, 0x87, 0x85,
      0xbb, 0xb9, 0xbf, 0xbd, 0xb3, 0xb1, 0xb7, 0xb5, 0xab, 0xa9, 0xaf, 0xad, 0xa3, 0xa1, 0xa7, 0xa5,
      0xdb, 0xd9, 0xdf, 0xdd, 0xd3, 0xd1, 0xd7, 0xd5, 0xcb, 0xc9, 0xcf, 0xcd, 0xc3, 0xc1, 0xc7, 0xc5,
      0xfb, 0xf9, 0xff, 0xfd, 0xf3, 0xf1, 0xf7, 0xf5, 0xeb, 0xe9, 0xef, 0xed, 0xe3, 0xe1, 0xe7, 0xe5,
    };
    return FFMul02[in];
  }

  static uint8_t FFMul03(uint8_t in) {
    static const uint8_t FFMul03[256] = {
      0x00, 0x03, 0x06, 0x05, 0x0c, 0x0f, 0x0a, 0x09, 0x18, 0x1b, 0x1e, 0x1d, 0x14, 0x17, 0x12, 0x11,
      0x30, 0x33, 0x36, 0x35, 0x3c, 0x3f, 0x3a, 0x39, 0x28, 0x2b, 0x2e, 0x2d, 0x24, 0x27, 0x22, 0x21,
      0x60, 0x63, 0x66, 0x65, 0x6c, 0x6f, 0x6a, 0x69, 0x78, 0x7b, 0x7e, 0x7d, 0x74, 0x77, 0x72, 0x71,
      0x50, 0x53, 0x56, 0x55, 0x5c, 0x5f, 0x5a, 0x59, 0x48, 0x4b, 0x4e, 0x4d, 0x44, 0x47, 0x42, 0x41,
      0xc0, 0xc3, 0xc6, 0xc5, 0xcc, 0xcf, 0xca, 0xc9, 0xd8, 0xdb, 0xde, 0xdd, 0xd4, 0xd7, 0xd2, 0xd1,
      0xf0, 0xf3, 0xf6, 0xf5, 0xfc, 0xff, 0xfa, 0xf9, 0xe8, 0xeb, 0xee, 0xed, 0xe4, 0xe7, 0xe2, 0xe1,
      0xa0, 0xa3, 0xa6, 0xa5, 0xac, 0xaf, 0xaa, 0xa9, 0xb8, 0xbb, 0xbe, 0xbd, 0xb4, 0xb7, 0xb2, 0xb1,
      0x90, 0x93, 0x96, 0x95, 0x9c, 0x9f, 0x9a, 0x99, 0x88, 0x8b, 0x8e, 0x8d, 0x84, 0x87, 0x82, 0x81,
      0x9b, 0x98, 0x9d, 0x9e, 0x97, 0x94, 0x91, 0x92, 0x83, 0x80, 0x85, 0x86, 0x8f, 0x8c, 0x89, 0x8a,
      0xab, 0xa8, 0xad, 0xae, 0xa7, 0xa4, 0xa1, 0xa2, 0xb3, 0xb0, 0xb5, 0xb6, 0xbf, 0xbc, 0xb9, 0xba,
      0xfb, 0xf8, 0xfd, 0xfe, 0xf7, 0xf4, 0xf1, 0xf2, 0xe3, 0xe0, 0xe5, 0xe6, 0xef, 0xec, 0xe9, 0xea,
      0xcb, 0xc8, 0xcd, 0xce, 0xc7, 0xc4, 0xc1, 0xc2, 0xd3, 0xd0, 0xd5, 0xd6, 0xdf, 0xdc, 0xd9, 0xda,
      0x5b, 0x58, 0x5d, 0x5e, 0x57, 0x54, 0x51, 0x52, 0x43, 0x40, 0x45, 0x46, 0x4f, 0x4c, 0x49, 0x4a,
      0x6b, 0x68, 0x6d, 0x6e, 0x67, 0x64, 0x61, 0x62, 0x73, 0x70, 0x75, 0x76, 0x7f, 0x7c, 0x79, 0x7a,
      0x3b, 0x38, 0x3d, 0x3e, 0x37, 0x34, 0x31, 0x32, 0x23, 0x20, 0x25, 0x26, 0x2f, 0x2c, 0x29, 0x2a,
      0x0b, 0x08, 0x0d, 0x0e, 0x07, 0x04, 0x01, 0x02, 0x13, 0x10, 0x15, 0x16, 0x1f, 0x1c, 0x19, 0x1a,
    };
    return FFMul03[in];
  }

  static __uint128_t MixColumns(uint8_t *State) {
    uint8_t In0[16] = {
      State[0], State[4], State[8], State[12],
      State[1], State[5], State[9], State[13],
      State[2], State[6], State[10], State[14],
      State[3], State[7], State[11], State[15],
    };

    uint8_t Out0[4]{};
    uint8_t Out1[4]{};
    uint8_t Out2[4]{};
    uint8_t Out3[4]{};

    for (size_t i = 0; i < 4; ++i) {
      Out0[i] = FFMul02(In0[0 + i]) ^ FFMul03(In0[4 + i]) ^ In0[8 + i] ^ In0[12 + i];
      Out1[i] = In0[0 + i] ^ FFMul02(In0[4 + i]) ^ FFMul03(In0[8 + i]) ^ In0[12 + i];
      Out2[i] = In0[0 + i] ^ In0[4 + i] ^ FFMul02(In0[8 + i]) ^ FFMul03(In0[12 + i]);
      Out3[i] = FFMul03(In0[0 + i]) ^ In0[4 + i] ^ In0[8 + i] ^ FFMul02(In0[12 + i]);
    }

    uint8_t OutArray[16] = {
      Out0[0], Out1[0], Out2[0], Out3[0],
      Out0[1], Out1[1], Out2[1], Out3[1],
      Out0[2], Out1[2], Out2[2], Out3[2],
      Out0[3], Out1[3], Out2[3], Out3[3],
    };
    __uint128_t Res{};
    memcpy(&Res, OutArray, 16);
    return Res;
  }

  static uint8_t FFMul09(uint8_t in) {
    static const uint8_t FFMul09[256] = {
      0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f, 0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
      0x90, 0x99, 0x82, 0x8b, 0xb4, 0xbd, 0xa6, 0xaf, 0xd8, 0xd1, 0xca, 0xc3, 0xfc, 0xf5, 0xee, 0xe7,
      0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04, 0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c,
      0xab, 0xa2, 0xb9, 0xb0, 0x8f, 0x86, 0x9d, 0x94, 0xe3, 0xea, 0xf1, 0xf8, 0xc7, 0xce, 0xd5, 0xdc,
      0x76, 0x7f, 0x64, 0x6d, 0x52, 0x5b, 0x40, 0x49, 0x3e, 0x37, 0x2c, 0x25, 0x1a, 0x13, 0x08, 0x01,
      0xe6, 0xef, 0xf4, 0xfd, 0xc2, 0xcb, 0xd0, 0xd9, 0xae, 0xa7, 0xbc, 0xb5, 0x8a, 0x83, 0x98, 0x91,
      0x4d, 0x44, 0x5f, 0x56, 0x69, 0x60, 0x7b, 0x72, 0x05, 0x0c, 0x17, 0x1e, 0x21, 0x28, 0x33, 0x3a,
      0xdd, 0xd4, 0xcf, 0xc6, 0xf9, 0xf0, 0xeb, 0xe2, 0x95, 0x9c, 0x87, 0x8e, 0xb1, 0xb8, 0xa3, 0xaa,
      0xec, 0xe5, 0xfe, 0xf7, 0xc8, 0xc1, 0xda, 0xd3, 0xa4, 0xad, 0xb6, 0xbf, 0x80, 0x89, 0x92, 0x9b,
      0x7c, 0x75, 0x6e, 0x67, 0x58, 0x51, 0x4a, 0x43, 0x34, 0x3d, 0x26, 0x2f, 0x10, 0x19, 0x02, 0x0b,
      0xd7, 0xde, 0xc5, 0xcc, 0xf3, 0xfa, 0xe1, 0xe8, 0x9f, 0x96, 0x8d, 0x84, 0xbb, 0xb2, 0xa9, 0xa0,
      0x47, 0x4e, 0x55, 0x5c, 0x63, 0x6a, 0x71, 0x78, 0x0f, 0x06, 0x1d, 0x14, 0x2b, 0x22, 0x39, 0x30,
      0x9a, 0x93, 0x88, 0x81, 0xbe, 0xb7, 0xac, 0xa5, 0xd2, 0xdb, 0xc0, 0xc9, 0xf6, 0xff, 0xe4, 0xed,
      0x0a, 0x03, 0x18, 0x11, 0x2e, 0x27, 0x3c, 0x35, 0x42, 0x4b, 0x50, 0x59, 0x66, 0x6f, 0x74, 0x7d,
      0xa1, 0xa8, 0xb3, 0xba, 0x85, 0x8c, 0x97, 0x9e, 0xe9, 0xe0, 0xfb, 0xf2, 0xcd, 0xc4, 0xdf, 0xd6,
      0x31, 0x38, 0x23, 0x2a, 0x15, 0x1c, 0x07, 0x0e, 0x79, 0x70, 0x6b, 0x62, 0x5d, 0x54, 0x4f, 0x46,
    };
    return FFMul09[in];
  }

  static uint8_t FFMul0B(uint8_t in) {
    static const uint8_t FFMul0B[256] = {
      0x00, 0x0b, 0x16, 0x1d, 0x2c, 0x27, 0x3a, 0x31, 0x58, 0x53, 0x4e, 0x45, 0x74, 0x7f, 0x62, 0x69,
      0xb0, 0xbb, 0xa6, 0xad, 0x9c, 0x97, 0x8a, 0x81, 0xe8, 0xe3, 0xfe, 0xf5, 0xc4, 0xcf, 0xd2, 0xd9,
      0x7b, 0x70, 0x6d, 0x66, 0x57, 0x5c, 0x41, 0x4a, 0x23, 0x28, 0x35, 0x3e, 0x0f, 0x04, 0x19, 0x12,
      0xcb, 0xc0, 0xdd, 0xd6, 0xe7, 0xec, 0xf1, 0xfa, 0x93, 0x98, 0x85, 0x8e, 0xbf, 0xb4, 0xa9, 0xa2,
      0xf6, 0xfd, 0xe0, 0xeb, 0xda, 0xd1, 0xcc, 0xc7, 0xae, 0xa5, 0xb8, 0xb3, 0x82, 0x89, 0x94, 0x9f,
      0x46, 0x4d, 0x50, 0x5b, 0x6a, 0x61, 0x7c, 0x77, 0x1e, 0x15, 0x08, 0x03, 0x32, 0x39, 0x24, 0x2f,
      0x8d, 0x86, 0x9b, 0x90, 0xa1, 0xaa, 0xb7, 0xbc, 0xd5, 0xde, 0xc3, 0xc8, 0xf9, 0xf2, 0xef, 0xe4,
      0x3d, 0x36, 0x2b, 0x20, 0x11, 0x1a, 0x07, 0x0c, 0x65, 0x6e, 0x73, 0x78, 0x49, 0x42, 0x5f, 0x54,
      0xf7, 0xfc, 0xe1, 0xea, 0xdb, 0xd0, 0xcd, 0xc6, 0xaf, 0xa4, 0xb9, 0xb2, 0x83, 0x88, 0x95, 0x9e,
      0x47, 0x4c, 0x51, 0x5a, 0x6b, 0x60, 0x7d, 0x76, 0x1f, 0x14, 0x09, 0x02, 0x33, 0x38, 0x25, 0x2e,
      0x8c, 0x87, 0x9a, 0x91, 0xa0, 0xab, 0xb6, 0xbd, 0xd4, 0xdf, 0xc2, 0xc9, 0xf8, 0xf3, 0xee, 0xe5,
      0x3c, 0x37, 0x2a, 0x21, 0x10, 0x1b, 0x06, 0x0d, 0x64, 0x6f, 0x72, 0x79, 0x48, 0x43, 0x5e, 0x55,
      0x01, 0x0a, 0x17, 0x1c, 0x2d, 0x26, 0x3b, 0x30, 0x59, 0x52, 0x4f, 0x44, 0x75, 0x7e, 0x63, 0x68,
      0xb1, 0xba, 0xa7, 0xac, 0x9d, 0x96, 0x8b, 0x80, 0xe9, 0xe2, 0xff, 0xf4, 0xc5, 0xce, 0xd3, 0xd8,
      0x7a, 0x71, 0x6c, 0x67, 0x56, 0x5d, 0x40, 0x4b, 0x22, 0x29, 0x34, 0x3f, 0x0e, 0x05, 0x18, 0x13,
      0xca, 0xc1, 0xdc, 0xd7, 0xe6, 0xed, 0xf0, 0xfb, 0x92, 0x99, 0x84, 0x8f, 0xbe, 0xb5, 0xa8, 0xa3,
    };
    return FFMul0B[in];
  }

  static uint8_t FFMul0D(uint8_t in) {
    static const uint8_t FFMul0D[256] = {
      0x00, 0x0d, 0x1a, 0x17, 0x34, 0x39, 0x2e, 0x23, 0x68, 0x65, 0x72, 0x7f, 0x5c, 0x51, 0x46, 0x4b,
      0xd0, 0xdd, 0xca, 0xc7, 0xe4, 0xe9, 0xfe, 0xf3, 0xb8, 0xb5, 0xa2, 0xaf, 0x8c, 0x81, 0x96, 0x9b,
      0xbb, 0xb6, 0xa1, 0xac, 0x8f, 0x82, 0x95, 0x98, 0xd3, 0xde, 0xc9, 0xc4, 0xe7, 0xea, 0xfd, 0xf0,
      0x6b, 0x66, 0x71, 0x7c, 0x5f, 0x52, 0x45, 0x48, 0x03, 0x0e, 0x19, 0x14, 0x37, 0x3a, 0x2d, 0x20,
      0x6d, 0x60, 0x77, 0x7a, 0x59, 0x54, 0x43, 0x4e, 0x05, 0x08, 0x1f, 0x12, 0x31, 0x3c, 0x2b, 0x26,
      0xbd, 0xb0, 0xa7, 0xaa, 0x89, 0x84, 0x93, 0x9e, 0xd5, 0xd8, 0xcf, 0xc2, 0xe1, 0xec, 0xfb, 0xf6,
      0xd6, 0xdb, 0xcc, 0xc1, 0xe2, 0xef, 0xf8, 0xf5, 0xbe, 0xb3, 0xa4, 0xa9, 0x8a, 0x87, 0x90, 0x9d,
      0x06, 0x0b, 0x1c, 0x11, 0x32, 0x3f, 0x28, 0x25, 0x6e, 0x63, 0x74, 0x79, 0x5a, 0x57, 0x40, 0x4d,
      0xda, 0xd7, 0xc0, 0xcd, 0xee, 0xe3, 0xf4, 0xf9, 0xb2, 0xbf, 0xa8, 0xa5, 0x86, 0x8b, 0x9c, 0x91,
      0x0a, 0x07, 0x10, 0x1d, 0x3e, 0x33, 0x24, 0x29, 0x62, 0x6f, 0x78, 0x75, 0x56, 0x5b, 0x4c, 0x41,
      0x61, 0x6c, 0x7b, 0x76, 0x55, 0x58, 0x4f, 0x42, 0x09, 0x04, 0x13, 0x1e, 0x3d, 0x30, 0x27, 0x2a,
      0xb1, 0xbc, 0xab, 0xa6, 0x85, 0x88, 0x9f, 0x92, 0xd9, 0xd4, 0xc3, 0xce, 0xed, 0xe0, 0xf7, 0xfa,
      0xb7, 0xba, 0xad, 0xa0, 0x83, 0x8e, 0x99, 0x94, 0xdf, 0xd2, 0xc5, 0xc8, 0xeb, 0xe6, 0xf1, 0xfc,
      0x67, 0x6a, 0x7d, 0x70, 0x53, 0x5e, 0x49, 0x44, 0x0f, 0x02, 0x15, 0x18, 0x3b, 0x36, 0x21, 0x2c,
      0x0c, 0x01, 0x16, 0x1b, 0x38, 0x35, 0x22, 0x2f, 0x64, 0x69, 0x7e, 0x73, 0x50, 0x5d, 0x4a, 0x47,
      0xdc, 0xd1, 0xc6, 0xcb, 0xe8, 0xe5, 0xf2, 0xff, 0xb4, 0xb9, 0xae, 0xa3, 0x80, 0x8d, 0x9a, 0x97,
    };

    return FFMul0D[in];
  }

  static uint8_t FFMul0E(uint8_t in) {
    static const uint8_t FFMul0E[256] = {
      0x00, 0x0e, 0x1c, 0x12, 0x38, 0x36, 0x24, 0x2a, 0x70, 0x7e, 0x6c, 0x62, 0x48, 0x46, 0x54, 0x5a,
      0xe0, 0xee, 0xfc, 0xf2, 0xd8, 0xd6, 0xc4, 0xca, 0x90, 0x9e, 0x8c, 0x82, 0xa8, 0xa6, 0xb4, 0xba,
      0xdb, 0xd5, 0xc7, 0xc9, 0xe3, 0xed, 0xff, 0xf1, 0xab, 0xa5, 0xb7, 0xb9, 0x93, 0x9d, 0x8f, 0x81,
      0x3b, 0x35, 0x27, 0x29, 0x03, 0x0d, 0x1f, 0x11, 0x4b, 0x45, 0x57, 0x59, 0x73, 0x7d, 0x6f, 0x61,
      0xad, 0xa3, 0xb1, 0xbf, 0x95, 0x9b, 0x89, 0x87, 0xdd, 0xd3, 0xc1, 0xcf, 0xe5, 0xeb, 0xf9, 0xf7,
      0x4d, 0x43, 0x51, 0x5f, 0x75, 0x7b, 0x69, 0x67, 0x3d, 0x33, 0x21, 0x2f, 0x05, 0x0b, 0x19, 0x17,
      0x76, 0x78, 0x6a, 0x64, 0x4e, 0x40, 0x52, 0x5c, 0x06, 0x08, 0x1a, 0x14, 0x3e, 0x30, 0x22, 0x2c,
      0x96, 0x98, 0x8a, 0x84, 0xae, 0xa0, 0xb2, 0xbc, 0xe6, 0xe8, 0xfa, 0xf4, 0xde, 0xd0, 0xc2, 0xcc,
      0x41, 0x4f, 0x5d, 0x53, 0x79, 0x77, 0x65, 0x6b, 0x31, 0x3f, 0x2d, 0x23, 0x09, 0x07, 0x15, 0x1b,
      0xa1, 0xaf, 0xbd, 0xb3, 0x99, 0x97, 0x85, 0x8b, 0xd1, 0xdf, 0xcd, 0xc3, 0xe9, 0xe7, 0xf5, 0xfb,
      0x9a, 0x94, 0x86, 0x88, 0xa2, 0xac, 0xbe, 0xb0, 0xea, 0xe4, 0xf6, 0xf8, 0xd2, 0xdc, 0xce, 0xc0,
      0x7a, 0x74, 0x66, 0x68, 0x42, 0x4c, 0x5e, 0x50, 0x0a, 0x04, 0x16, 0x18, 0x32, 0x3c, 0x2e, 0x20,
      0xec, 0xe2, 0xf0, 0xfe, 0xd4, 0xda, 0xc8, 0xc6, 0x9c, 0x92, 0x80, 0x8e, 0xa4, 0xaa, 0xb8, 0xb6,
      0x0c, 0x02, 0x10, 0x1e, 0x34, 0x3a, 0x28, 0x26, 0x7c, 0x72, 0x60, 0x6e, 0x44, 0x4a, 0x58, 0x56,
      0x37, 0x39, 0x2b, 0x25, 0x0f, 0x01, 0x13, 0x1d, 0x47, 0x49, 0x5b, 0x55, 0x7f, 0x71, 0x63, 0x6d,
      0xd7, 0xd9, 0xcb, 0xc5, 0xef, 0xe1, 0xf3, 0xfd, 0xa7, 0xa9, 0xbb, 0xb5, 0x9f, 0x91, 0x83, 0x8d,
    };

    return FFMul0E[in];
  }

  static __uint128_t InvMixColumns(uint8_t *State) {
    uint8_t In0[16] = {
      State[0], State[4], State[8], State[12],
      State[1], State[5], State[9], State[13],
      State[2], State[6], State[10], State[14],
      State[3], State[7], State[11], State[15],
    };

    uint8_t Out0[4]{};
    uint8_t Out1[4]{};
    uint8_t Out2[4]{};
    uint8_t Out3[4]{};

    for (size_t i = 0; i < 4; ++i) {
      Out0[i] = FFMul0E(In0[0 + i]) ^ FFMul0B(In0[4 + i]) ^ FFMul0D(In0[8 + i]) ^ FFMul09(In0[12 + i]);
      Out1[i] = FFMul09(In0[0 + i]) ^ FFMul0E(In0[4 + i]) ^ FFMul0B(In0[8 + i]) ^ FFMul0D(In0[12 + i]);
      Out2[i] = FFMul0D(In0[0 + i]) ^ FFMul09(In0[4 + i]) ^ FFMul0E(In0[8 + i]) ^ FFMul0B(In0[12 + i]);
      Out3[i] = FFMul0B(In0[0 + i]) ^ FFMul0D(In0[4 + i]) ^ FFMul09(In0[8 + i]) ^ FFMul0E(In0[12 + i]);
    }

    uint8_t OutArray[16] = {
      Out0[0], Out1[0], Out2[0], Out3[0],
      Out0[1], Out1[1], Out2[1], Out3[1],
      Out0[2], Out1[2], Out2[2], Out3[2],
      Out0[3], Out1[3], Out2[3], Out3[3],
    };
    __uint128_t Res{};
    memcpy(&Res, OutArray, 16);
    return Res;
  }
}

template<typename unsigned_type, typename signed_type, typename float_type>
bool IsConditionTrue(uint8_t Cond, uint64_t Src1, uint64_t Src2) {
  bool CompResult = false;
  switch (Cond) {
    case FEXCore::IR::COND_EQ:
      CompResult = static_cast<unsigned_type>(Src1) == static_cast<unsigned_type>(Src2);
      break;
    case FEXCore::IR::COND_NEQ:
      CompResult = static_cast<unsigned_type>(Src1) != static_cast<unsigned_type>(Src2);
      break;
    case FEXCore::IR::COND_SGE:
      CompResult = static_cast<signed_type>(Src1) >= static_cast<signed_type>(Src2);
      break;
    case FEXCore::IR::COND_SLT:
      CompResult = static_cast<signed_type>(Src1) < static_cast<signed_type>(Src2);
      break;
    case FEXCore::IR::COND_SGT:
      CompResult = static_cast<signed_type>(Src1) > static_cast<signed_type>(Src2);
      break;
    case FEXCore::IR::COND_SLE:
      CompResult = static_cast<signed_type>(Src1) <= static_cast<signed_type>(Src2);
      break;
    case FEXCore::IR::COND_UGE:
      CompResult = static_cast<unsigned_type>(Src1) >= static_cast<unsigned_type>(Src2);
      break;
    case FEXCore::IR::COND_ULT:
      CompResult = static_cast<unsigned_type>(Src1) < static_cast<unsigned_type>(Src2);
      break;
    case FEXCore::IR::COND_UGT:
      CompResult = static_cast<unsigned_type>(Src1) > static_cast<unsigned_type>(Src2);
      break;
    case FEXCore::IR::COND_ULE:
      CompResult = static_cast<unsigned_type>(Src1) <= static_cast<unsigned_type>(Src2);
      break;

    case FEXCore::IR::COND_FLU:
      CompResult = reinterpret_cast<float_type&>(Src1) < reinterpret_cast<float_type&>(Src2) || (std::isnan(reinterpret_cast<float_type&>(Src1)) || std::isnan(reinterpret_cast<float_type&>(Src2)));
      break;
    case FEXCore::IR::COND_FGE:
      CompResult = reinterpret_cast<float_type&>(Src1) >= reinterpret_cast<float_type&>(Src2) && !(std::isnan(reinterpret_cast<float_type&>(Src1)) || std::isnan(reinterpret_cast<float_type&>(Src2)));
      break;
    case FEXCore::IR::COND_FLEU:
      CompResult = reinterpret_cast<float_type&>(Src1) <= reinterpret_cast<float_type&>(Src2) || (std::isnan(reinterpret_cast<float_type&>(Src1)) || std::isnan(reinterpret_cast<float_type&>(Src2)));
      break;
    case FEXCore::IR::COND_FGT:
      CompResult = reinterpret_cast<float_type&>(Src1) > reinterpret_cast<float_type&>(Src2) && !(std::isnan(reinterpret_cast<float_type&>(Src1)) || std::isnan(reinterpret_cast<float_type&>(Src2)));
      break;
    case FEXCore::IR::COND_FU:
      CompResult = (std::isnan(reinterpret_cast<float_type&>(Src1)) || std::isnan(reinterpret_cast<float_type&>(Src2)));
      break;
    case FEXCore::IR::COND_FNU:
      CompResult = !(std::isnan(reinterpret_cast<float_type&>(Src1)) || std::isnan(reinterpret_cast<float_type&>(Src2)));
      break;
    case FEXCore::IR::COND_MI:
    case FEXCore::IR::COND_PL:
    case FEXCore::IR::COND_VS:
    case FEXCore::IR::COND_VC:
    default:
      LogMan::Msg::A("Unsupported compare type");
      break;
  }

  return CompResult;
}

template<typename Res>
Res GetDest(void* SSAData, IR::OrderedNodeWrapper Op) {
  auto DstPtr = &reinterpret_cast<__uint128_t*>(SSAData)[Op.ID()];
  return reinterpret_cast<Res>(DstPtr);
}

template<typename Res>
Res GetSrc(void* SSAData, IR::OrderedNodeWrapper Src) {
  auto DstPtr = &reinterpret_cast<__uint128_t*>(SSAData)[Src.ID()];
  return reinterpret_cast<Res>(DstPtr);
}

[[noreturn]]
static void StopThread(FEXCore::Core::InternalThreadState *Thread) {
  Thread->CTX->StopThread(Thread);

  LogMan::Msg::A("unreachable");
  __builtin_unreachable();
}

[[noreturn]]
static void SignalReturn(FEXCore::Core::InternalThreadState *Thread) {
  Thread->CTX->SignalThread(Thread, FEXCore::Core::SIGNALEVENT_RETURN);

  LogMan::Msg::A("unreachable");
  __builtin_unreachable();
}

template<IR::IROps Op>
struct OpHandlers {

};

template<>
struct OpHandlers<IR::OP_F80CVTTO> {
  static X80SoftFloat handle4(float src) {
    return src;
  }

  static X80SoftFloat handle8(double src) {
    return src;
  }
};

template<>
struct OpHandlers<IR::OP_F80CMP> {
  template<uint32_t Flags>
  static uint64_t handle(X80SoftFloat Src1, X80SoftFloat Src2) {
    bool eq, lt, nan;
    uint64_t ResultFlags = 0;

    X80SoftFloat::FCMP(Src1, Src2, &eq, &lt, &nan);
    if (Flags & (1 << IR::FCMP_FLAG_LT) &&
        lt) {
      ResultFlags |= (1 << IR::FCMP_FLAG_LT);
    }
    if (Flags & (1 << IR::FCMP_FLAG_UNORDERED) &&
        nan) {
      ResultFlags |= (1 << IR::FCMP_FLAG_UNORDERED);
    }
    if (Flags & (1 << IR::FCMP_FLAG_EQ) &&
        eq) {
      ResultFlags |= (1 << IR::FCMP_FLAG_EQ);
    }
    return ResultFlags;
  }
};

template<>
struct OpHandlers<IR::OP_F80CVT> {
  static float handle4(X80SoftFloat src) {
    return src;
  }

  static double handle8(X80SoftFloat src) {
    return src;
  }
};

template<>
struct OpHandlers<IR::OP_F80CVTINT> {
  static  int16_t handle2(X80SoftFloat src) {
    return src;
  }

  static int32_t handle4(X80SoftFloat src) {
    return src;
  }

  static int64_t handle8(X80SoftFloat src) {
    return src;
  }

  static  int16_t handle2t(X80SoftFloat src) {
    auto rv = extF80_to_i32(src, softfloat_round_minMag, false);

    if (rv > INT16_MAX) {
      return INT16_MAX;
    } else if (rv < INT16_MIN) {
      return INT16_MIN;
    } else {
      return rv;
    }
  }

  static int32_t handle4t(X80SoftFloat src) {
    return extF80_to_i32(src, softfloat_round_minMag, false);
  }

  static int64_t handle8t(X80SoftFloat src) {
    return extF80_to_i64(src, softfloat_round_minMag, false);
  }
};

template<>
struct OpHandlers<IR::OP_F80CVTTOINT> {
  static X80SoftFloat handle2(int16_t src) {
    return src;
  }

  static X80SoftFloat handle4(int32_t src) {
    return src;
  }
};

template<>
struct OpHandlers<IR::OP_F80ROUND> {
  static X80SoftFloat handle(X80SoftFloat Src1) {
    return X80SoftFloat::FRNDINT(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80F2XM1> {
  static X80SoftFloat handle(X80SoftFloat Src1) {
    return X80SoftFloat::F2XM1(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80TAN> {
  static X80SoftFloat handle(X80SoftFloat Src1) {
    return X80SoftFloat::FTAN(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80SQRT> {
  static X80SoftFloat handle(X80SoftFloat Src1) {
    return X80SoftFloat::FSQRT(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80SIN> {
  static X80SoftFloat handle(X80SoftFloat Src1) {
    return X80SoftFloat::FSIN(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80COS> {
  static X80SoftFloat handle(X80SoftFloat Src1) {
    return X80SoftFloat::FCOS(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80XTRACT_EXP> {
  static X80SoftFloat handle(X80SoftFloat Src1) {
    return X80SoftFloat::FXTRACT_EXP(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80XTRACT_SIG> {
  static X80SoftFloat handle(X80SoftFloat Src1) {
    return X80SoftFloat::FXTRACT_SIG(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80ADD> {
  static X80SoftFloat handle(X80SoftFloat Src1, X80SoftFloat Src2) {
    return X80SoftFloat::FADD(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80SUB> {
  static X80SoftFloat handle(X80SoftFloat Src1, X80SoftFloat Src2) {
    return X80SoftFloat::FSUB(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80MUL> {
  static X80SoftFloat handle(X80SoftFloat Src1, X80SoftFloat Src2) {
    return X80SoftFloat::FMUL(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80DIV> {
  static X80SoftFloat handle(X80SoftFloat Src1, X80SoftFloat Src2) {
    return X80SoftFloat::FDIV(Src1, Src2);
  }
};


template<>
struct OpHandlers<IR::OP_F80FYL2X> {
  static X80SoftFloat handle(X80SoftFloat Src1, X80SoftFloat Src2) {
    return X80SoftFloat::FYL2X(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80ATAN> {
  static X80SoftFloat handle(X80SoftFloat Src1, X80SoftFloat Src2) {
    return X80SoftFloat::FATAN(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80FPREM1> {
  static X80SoftFloat handle(X80SoftFloat Src1, X80SoftFloat Src2) {
    return X80SoftFloat::FREM1(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80FPREM> {
  static X80SoftFloat handle(X80SoftFloat Src1, X80SoftFloat Src2) {
    return X80SoftFloat::FREM(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80SCALE> {
  static X80SoftFloat handle(X80SoftFloat Src1, X80SoftFloat Src2) {
    return X80SoftFloat::FSCALE(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80BCDSTORE> {
  static X80SoftFloat handle(X80SoftFloat Src1) {
    bool Negative = Src1.Sign;

    // Clear the Sign bit
    Src1.Sign = 0;

    uint64_t Tmp = Src1;
    X80SoftFloat Rv;
    uint8_t *BCD = reinterpret_cast<uint8_t*>(&Rv);
    memset(BCD, 0, 10);

    for (size_t i = 0; i < 9; ++i) {
      if (Tmp == 0) {
        // Nothing left? Just leave
        break;
      }
      // Extract the lower 100 values
      uint8_t Digit = Tmp % 100;

      // Now divide it for the next iteration
      Tmp /= 100;

      uint8_t UpperNibble = Digit / 10;
      uint8_t LowerNibble = Digit % 10;

      // Now store the BCD
      BCD[i] = (UpperNibble << 4) | LowerNibble;
    }

    // Set negative flag once converted to x87
    BCD[9] = Negative ? 0x80 : 0;

    return Rv;
  }
};

template<>
struct OpHandlers<IR::OP_F80BCDLOAD> {
  static X80SoftFloat handle(X80SoftFloat Src) {
    uint8_t *Src1 = reinterpret_cast<uint8_t *>(&Src);
    uint64_t BCD{};
    // We walk through each uint8_t and pull out the BCD encoding
    // Each 4bit split is a digit
    // Only 0-9 is supported, A-F results in undefined data
    // | 4 bit     | 4 bit    |
    // | 10s place | 1s place |
    // EG 0x48 = 48
    // EG 0x4847 = 4847
    // This gives us an 18digit value encoded in BCD
    // The last byte lets us know if it negative or not
    for (size_t i = 0; i < 9; ++i) {
      uint8_t Digit = Src1[8 - i];
      // First shift our last value over
      BCD *= 100;

      // Add the tens place digit
      BCD += (Digit >> 4) * 10;

      // Add the ones place digit
      BCD += Digit & 0xF;
    }

    // Set negative flag once converted to x87
    bool Negative = Src1[9] & 0x80;
    X80SoftFloat Tmp;

    Tmp = BCD;
    Tmp.Sign = Negative;
    return Tmp;
  }
};

template<>
struct OpHandlers<IR::OP_F80LOADFCW> {
  static void handle(uint16_t NewFCW) {

    auto PC = (NewFCW >> 8) & 3;
    switch(PC) {
      case 0: extF80_roundingPrecision = 32; break;
      case 2: extF80_roundingPrecision = 64; break;
      case 3: extF80_roundingPrecision = 80; break;
      case 1: LogMan::Msg::A("Invalid x87 precision mode, %d", PC);
    }

    auto RC = (NewFCW >> 10) & 3;
    switch(RC) {
      case 0:
        softfloat_roundingMode = softfloat_round_near_even;
        break;
      case 1:
        softfloat_roundingMode = softfloat_round_min;
        break;
      case 2:
        softfloat_roundingMode = softfloat_round_max;
        break;
      case 3:
        softfloat_roundingMode = softfloat_round_minMag;
      break;
    }
  }
};

template<typename R, typename... Args>
FallbackInfo GetFallbackInfo(R(*fn)(Args...)) {
  return {FABI_UNKNOWN, (void*)fn};
}

template<>
FallbackInfo GetFallbackInfo(X80SoftFloat(*fn)(float)) {
  return {FABI_F80_F32, (void*)fn};
}

template<>
FallbackInfo GetFallbackInfo(X80SoftFloat(*fn)(double)) {
  return {FABI_F80_F64, (void*)fn};
}

template<>
FallbackInfo GetFallbackInfo(X80SoftFloat(*fn)(int16_t)) {
  return {FABI_F80_I16, (void*)fn};
}

template<>
FallbackInfo GetFallbackInfo(void(*fn)(uint16_t)) {
  return {FABI_VOID_U16, (void*)fn};
}

template<>
FallbackInfo GetFallbackInfo(X80SoftFloat(*fn)(int32_t)) {
  return {FABI_F80_I32, (void*)fn};
}

template<>
FallbackInfo GetFallbackInfo(float(*fn)(X80SoftFloat)) {
  return {FABI_F32_F80, (void*)fn};
}

template<>
FallbackInfo GetFallbackInfo(double(*fn)(X80SoftFloat)) {
  return {FABI_F64_F80, (void*)fn};
}

template<>
FallbackInfo GetFallbackInfo(int16_t(*fn)(X80SoftFloat)) {
  return {FABI_I16_F80, (void*)fn};
}

template<>
FallbackInfo GetFallbackInfo(int32_t(*fn)(X80SoftFloat)) {
  return {FABI_I32_F80, (void*)fn};
}

template<>
FallbackInfo GetFallbackInfo(int64_t(*fn)(X80SoftFloat)) {
  return {FABI_I64_F80, (void*)fn};
}

template<>
FallbackInfo GetFallbackInfo(uint64_t(*fn)(X80SoftFloat, X80SoftFloat)) {
  return {FABI_I64_F80_F80, (void*)fn};
}

template<>
FallbackInfo GetFallbackInfo(X80SoftFloat(*fn)(X80SoftFloat)) {
  return {FABI_F80_F80, (void*)fn};
}

template<>
FallbackInfo GetFallbackInfo(X80SoftFloat(*fn)(X80SoftFloat, X80SoftFloat)) {
  return {FABI_F80_F80_F80, (void*)fn};
}

bool InterpreterOps::GetFallbackHandler(IR::IROp_Header *IROp, FallbackInfo *Info) {
  uint8_t OpSize = IROp->Size;
  switch(IROp->Op) {
    case IR::OP_F80LOADFCW: {
      *Info = GetFallbackInfo(&OpHandlers<IR::OP_F80LOADFCW>::handle);
      return true;
    }

    case IR::OP_F80CVTTO: {
      auto Op = IROp->C<IR::IROp_F80CVTTo>();

      switch (Op->Size) {
        case 4: {
          *Info = GetFallbackInfo(&OpHandlers<IR::OP_F80CVTTO>::handle4);
          return true;
        }
        case 8: {
          *Info = GetFallbackInfo(&OpHandlers<IR::OP_F80CVTTO>::handle8);
          return true;
        }
      default: LogMan::Msg::D("Unhandled size: %d", OpSize);
      }
      break;
    }
    case IR::OP_F80CVT: {
      switch (OpSize) {
        case 4: {
          *Info = GetFallbackInfo(&OpHandlers<IR::OP_F80CVT>::handle4);
          return true;
        }
        case 8: {
          *Info = GetFallbackInfo(&OpHandlers<IR::OP_F80CVT>::handle8);
          return true;
        }
        default: LogMan::Msg::D("Unhandled size: %d", OpSize);
      }
      break;
    }
    case IR::OP_F80CVTINT: {
      auto Op = IROp->C<IR::IROp_F80CVTInt>();

      switch (OpSize) {
        case 2: {
          *Info = GetFallbackInfo(Op->Truncate ? &OpHandlers<IR::OP_F80CVTINT>::handle2t : &OpHandlers<IR::OP_F80CVTINT>::handle2);
          return true;
        }
        case 4: {
          *Info = GetFallbackInfo(Op->Truncate ? &OpHandlers<IR::OP_F80CVTINT>::handle4t : &OpHandlers<IR::OP_F80CVTINT>::handle4);
          return true;
        }
        case 8: {
          *Info = GetFallbackInfo(Op->Truncate ? &OpHandlers<IR::OP_F80CVTINT>::handle8t : &OpHandlers<IR::OP_F80CVTINT>::handle8);
          return true;
        }
        default: LogMan::Msg::D("Unhandled size: %d", OpSize);
      }
      break;
    }
    case IR::OP_F80CMP: {
      auto Op = IROp->C<IR::IROp_F80Cmp>();

      decltype(&OpHandlers<IR::OP_F80CMP>::handle<0>) handlers[] = { &OpHandlers<IR::OP_F80CMP>::handle<0>, &OpHandlers<IR::OP_F80CMP>::handle<1>, &OpHandlers<IR::OP_F80CMP>::handle<2>, &OpHandlers<IR::OP_F80CMP>::handle<3>, &OpHandlers<IR::OP_F80CMP>::handle<4>, &OpHandlers<IR::OP_F80CMP>::handle<5>, &OpHandlers<IR::OP_F80CMP>::handle<6>, &OpHandlers<IR::OP_F80CMP>::handle<7> };

      *Info = GetFallbackInfo(handlers[Op->Flags]);
      return true;
    }

    case IR::OP_F80CVTTOINT: {
      auto Op = IROp->C<IR::IROp_F80CVTToInt>();

      switch (Op->Size) {
        case 2: {
          *Info = GetFallbackInfo(&OpHandlers<IR::OP_F80CVTTOINT>::handle2);
          return true;
        }
        case 4: {
          *Info = GetFallbackInfo(&OpHandlers<IR::OP_F80CVTTOINT>::handle4);
          return true;
        }
        default: LogMan::Msg::D("Unhandled size: %d", OpSize);
      }
      break;
    }

#define COMMON_X87_OP(OP) \
    case IR::OP_F80##OP: { \
      *Info = GetFallbackInfo(&OpHandlers<IR::OP_F80##OP>::handle); \
      return true; \
    }

    // Unary
    COMMON_X87_OP(ROUND)
    COMMON_X87_OP(F2XM1)
    COMMON_X87_OP(TAN)
    COMMON_X87_OP(SQRT)
    COMMON_X87_OP(SIN)
    COMMON_X87_OP(COS)
    COMMON_X87_OP(XTRACT_EXP)
    COMMON_X87_OP(XTRACT_SIG)
    COMMON_X87_OP(BCDSTORE)
    COMMON_X87_OP(BCDLOAD)

    // Binary
    COMMON_X87_OP(ADD)
    COMMON_X87_OP(SUB)
    COMMON_X87_OP(MUL)
    COMMON_X87_OP(DIV)
    COMMON_X87_OP(FYL2X)
    COMMON_X87_OP(ATAN)
    COMMON_X87_OP(FPREM1)
    COMMON_X87_OP(FPREM)
    COMMON_X87_OP(SCALE)

    default:
      break;
  }

  return false;
}

void InterpreterOps::InterpretIR(FEXCore::Core::InternalThreadState *Thread, uint64_t Entry, FEXCore::IR::IRListView *CurrentIR, FEXCore::Core::DebugData *DebugData) {
  volatile void* stack = alloca(0);

  // Debug data is only passed in debug builds
  #ifndef NDEBUG
  // TODO: should be moved to an IR Op
  Thread->Stats.InstructionsExecuted.fetch_add(DebugData->GuestInstructionCount);
  #endif

  uintptr_t ListSize = CurrentIR->GetSSACount();

  uintptr_t ListBegin = CurrentIR->GetListData();
  uintptr_t DataBegin = CurrentIR->GetData();

  static_assert(sizeof(FEXCore::IR::IROp_Header) == 4);
  static_assert(sizeof(FEXCore::IR::OrderedNode) == 16);

  auto BlockIterator = CurrentIR->GetBlocks().begin();
  auto BlockEnd = CurrentIR->GetBlocks().end();

  // Allocate 16 bytes per SSA
  void *SSAData = alloca(ListSize * 16);

  // Clear them all to zero. Required for Zero-extend semantics
  memset(SSAData, 0, ListSize * 16);

#define GD *GetDest<uint64_t*>(SSAData, WrapperOp)
#define GDP GetDest<void*>(SSAData, WrapperOp)
  auto GetOpSize = [&](IR::OrderedNodeWrapper Node) {
    auto IROp = CurrentIR->GetOp<FEXCore::IR::IROp_Header>(Node);
    return IROp->Size;
  };

  while (1) {
    using namespace FEXCore::IR;
    auto [BlockNode, BlockHeader] = BlockIterator();
    auto BlockIROp = BlockHeader->CW<IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR->at(BlockIROp->Begin);
    auto CodeLast = CurrentIR->at(BlockIROp->Last);
    struct {
      bool Quit;
      bool Redo;
    } BlockResults{};

    auto HandleBlock = [&](OrderedNode *BlockNode) {
      for (auto [CodeNode, IROp] : CurrentIR->GetCode(BlockNode)) {
        OrderedNodeWrapper WrapperOp = CodeNode->Wrapped(ListBegin);
        uint8_t OpSize = IROp->Size;

        switch (IROp->Op) {
          case IR::OP_VALIDATECODE: {
            auto Op = IROp->C<IR::IROp_ValidateCode>();

            auto CodePtr = Entry + Op->Offset;
            if (memcmp((void*)CodePtr, &Op->CodeOriginalLow, Op->CodeLength) != 0) {
              GD = 1;
            } else {
              GD = 0;
            }
            break;
          }

          case IR::OP_REMOVECODEENTRY: {
            Thread->CTX->RemoveCodeEntry(Thread, Entry);
            break;
          }

          case IR::OP_DUMMY:
          case IR::OP_BEGINBLOCK:
          case IR::OP_ENDBLOCK:
          case IR::OP_INVALIDATEFLAGS:
            break;
          case IR::OP_FENCE: {
            auto Op = IROp->C<IR::IROp_Fence>();
            switch (Op->Fence) {
              case IR::Fence_Load.Val:
                std::atomic_thread_fence(std::memory_order_acquire);
                break;
              case IR::Fence_LoadStore.Val:
                std::atomic_thread_fence(std::memory_order_seq_cst);
                break;
              case IR::Fence_Store.Val:
                std::atomic_thread_fence(std::memory_order_release);
                break;
              default: LogMan::Msg::A("Unknown Fence: %d", Op->Fence); break;
            }
            break;
          }
          case IR::OP_EXITFUNCTION: {
            auto Op = IROp->C<IR::IROp_ExitFunction>();
            uintptr_t* ContextPtr = reinterpret_cast<uintptr_t*>(Thread->CurrentFrame);

            void *Data = reinterpret_cast<void*>(ContextPtr);
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);

            memcpy(Data, Src, OpSize);

            BlockResults.Quit = true;
            return;
            break;
          }
          case IR::OP_CONDJUMP: {
            auto Op = IROp->C<IR::IROp_CondJump>();
            bool CompResult;

            uint64_t Src1 = *GetSrc<uint64_t*>(SSAData, Op->Cmp1);
            uint64_t Src2 = *GetSrc<uint64_t*>(SSAData, Op->Cmp2);

            if (Op->CompareSize == 4)
              CompResult = IsConditionTrue<uint32_t, int32_t, float>(Op->Cond.Val, Src1, Src2);
            else
              CompResult = IsConditionTrue<uint64_t, int64_t, double>(Op->Cond.Val, Src1, Src2);

            if (CompResult) {
              BlockIterator = NodeIterator(ListBegin, DataBegin, Op->TrueBlock);
            }
            else  {
              BlockIterator = NodeIterator(ListBegin, DataBegin, Op->FalseBlock);
            }
            BlockResults.Redo = true;
            return;
            break;
          }
          case IR::OP_JUMP: {
            auto Op = IROp->C<IR::IROp_Jump>();
            BlockIterator = NodeIterator(ListBegin, DataBegin, Op->Header.Args[0]);
            BlockResults.Redo = true;
            return;
            break;
          }
          case IR::OP_BREAK: {
            auto Op = IROp->C<IR::IROp_Break>();
            switch (Op->Reason) {
              case 4: // HLT
                StopThread(Thread);
              break;
            default: LogMan::Msg::A("Unknown Break Reason: %d", Op->Reason); break;
            }
            break;
          }
          case IR::OP_SIGNALRETURN: {
            SignalReturn(Thread);
            break;
          }
          case IR::OP_CALLBACKRETURN: {
            Thread->CTX->InterpreterCallbackReturn(Thread, stack);
            break;
          }
          case IR::OP_SYSCALL: {
            auto Op = IROp->C<IR::IROp_Syscall>();

            FEXCore::HLE::SyscallArguments Args;
            for (size_t j = 0; j < FEXCore::HLE::SyscallArguments::MAX_ARGS; ++j) {
              if (Op->Header.Args[j].IsInvalid()) break;
              Args.Argument[j] = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[j]);
            }

            uint64_t Res = FEXCore::Context::HandleSyscall(Thread->CTX->SyscallHandler, Thread->CurrentFrame, &Args);
            GD = Res;
            break;
          }
          case IR::OP_THUNK: {
            auto Op = IROp->C<IR::IROp_Thunk>();

            auto thunkFn = Thread->CTX->ThunkHandler->LookupThunk(Op->ThunkNameHash);
            thunkFn(*GetSrc<void**>(SSAData, Op->Header.Args[0]));
            break;
          }
          case IR::OP_CPUID: {
            auto Op = IROp->C<IR::IROp_CPUID>();
            uint64_t *DstPtr = GetDest<uint64_t*>(SSAData, WrapperOp);
            uint64_t Arg = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
            uint64_t Leaf = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);

            auto Results = Thread->CTX->CPUID.RunFunction(Arg, Leaf);
            memcpy(DstPtr, &Results, sizeof(uint32_t) * 4);
            break;
          }
          case IR::OP_PRINT: {
            auto Op = IROp->C<IR::IROp_Print>();

            if (OpSize <= 8) {
              uint64_t Src = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
              LogMan::Msg::I(">>>> Value in Arg: 0x%lx, %ld", Src, Src);
            }
            else if (OpSize == 16) {
              __uint128_t Src = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]);
              uint64_t Src0 = Src;
              uint64_t Src1 = Src >> 64;
              LogMan::Msg::I(">>>> Value[0] in Arg: 0x%lx, %ld", Src0, Src0);
              LogMan::Msg::I("     Value[1] in Arg: 0x%lx, %ld", Src1, Src1);
            }
            else
              LogMan::Msg::A("Unknown value size: %d", OpSize);
            break;
          }
          case IR::OP_CYCLECOUNTER: {
            #ifdef DEBUG_CYCLES
              GD = 0;
            #else
              timespec time;
              clock_gettime(CLOCK_REALTIME, &time);
              GD = time.tv_nsec + time.tv_sec * 1000000000;
            #endif
            break;
          }
          case IR::OP_MOV: {
            auto Op = IROp->C<IR::IROp_Mov>();
            memcpy(GDP, GetSrc<void*>(SSAData, Op->Header.Args[0]), OpSize);
            break;
          }
          case IR::OP_VBITCAST: {
            auto Op = IROp->C<IR::IROp_VBitcast>();
            memcpy(GDP, GetSrc<void*>(SSAData, Op->Header.Args[0]), 16);
            break;
          }
          case IR::OP_VCASTFROMGPR: {
            auto Op = IROp->C<IR::IROp_VCastFromGPR>();
            memcpy(GDP, GetSrc<void*>(SSAData, Op->Header.Args[0]), Op->Header.ElementSize);
            break;
          }
          case IR::OP_VEXTRACTTOGPR: {
            auto Op = IROp->C<IR::IROp_VExtractToGPR>();
            uint32_t SourceSize = GetOpSize(Op->Header.Args[0]);

            LogMan::Throw::A(OpSize <= 16, "OpSize is too large for VExtractToGPR: %d", OpSize);

            if (SourceSize == 16) {
              __uint128_t SourceMask = (1ULL << (Op->Header.ElementSize * 8)) - 1;
              uint64_t Shift = Op->Header.ElementSize * Op->Idx * 8;
              if (Op->Header.ElementSize == 8)
                SourceMask = ~0ULL;

              __uint128_t Src = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]);
              Src >>= Shift;
              Src &= SourceMask;
              memcpy(GDP, &Src, Op->Header.ElementSize);
            }
            else {
              uint64_t SourceMask = (1ULL << (Op->Header.ElementSize * 8)) - 1;
              uint64_t Shift = Op->Header.ElementSize * Op->Idx * 8;
              if (Op->Header.ElementSize == 8)
                SourceMask = ~0ULL;

              uint64_t Src = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
              Src >>= Shift;
              Src &= SourceMask;
              GD = Src;
            }
            break;
          }
          case IR::OP_VEXTRACTELEMENT: {
            auto Op = IROp->C<IR::IROp_VExtractElement>();
            uint32_t SourceSize = GetOpSize(Op->Header.Args[0]);
            LogMan::Throw::A(OpSize <= 16, "OpSize is too large for VExtractToGPR: %d", OpSize);
            if (SourceSize == 16) {
              __uint128_t SourceMask = (1ULL << (Op->Header.ElementSize * 8)) - 1;
              uint64_t Shift = Op->Header.ElementSize * Op->Index * 8;
              if (Op->Header.ElementSize == 8)
                SourceMask = ~0ULL;

              __uint128_t Src = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]);
              Src >>= Shift;
              Src &= SourceMask;
              memcpy(GDP, &Src, Op->Header.ElementSize);
            }
            else {
              uint64_t SourceMask = (1ULL << (Op->Header.ElementSize * 8)) - 1;
              uint64_t Shift = Op->Header.ElementSize * Op->Index * 8;
              if (Op->Header.ElementSize == 8)
                SourceMask = ~0ULL;

              uint64_t Src = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
              Src >>= Shift;
              Src &= SourceMask;
              GD = Src;
            }
            break;
          }

          case IR::OP_ENTRYPOINTOFFSET: {
            auto Op = IROp->C<IR::IROp_EntrypointOffset>();
            GD = Entry + Op->Offset;
            break;
          }
          case IR::OP_CONSTANT: {
            auto Op = IROp->C<IR::IROp_Constant>();
            GD = Op->Constant;
            break;
          }
          case IR::OP_VECTORZERO: {
            memset(GDP, 0, OpSize);
            break;
          }
          case IR::OP_LOADCONTEXT: {
            auto Op = IROp->C<IR::IROp_LoadContext>();

            uintptr_t ContextPtr = reinterpret_cast<uintptr_t>(Thread->CurrentFrame);
            ContextPtr += Op->Offset;
            #define LOAD_CTX(x, y) \
              case x: { \
                y const *Data = reinterpret_cast<y const*>(ContextPtr); \
                GD = *Data; \
                break; \
              }
            switch (OpSize) {
              LOAD_CTX(1, uint8_t)
              LOAD_CTX(2, uint16_t)
              LOAD_CTX(4, uint32_t)
              LOAD_CTX(8, uint64_t)
              case 16: {
                void const *Data = reinterpret_cast<void const*>(ContextPtr);
                memcpy(GDP, Data, OpSize);
                break;
              }
              default:  LogMan::Msg::A("Unhandled LoadContext size: %d", OpSize);
            }
            #undef LOAD_CTX
            break;
          }
          case IR::OP_LOADCONTEXTINDEXED: {
            auto Op = IROp->C<IR::IROp_LoadContextIndexed>();
            uint64_t Index = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);

            uintptr_t ContextPtr = reinterpret_cast<uintptr_t>(Thread->CurrentFrame);

            ContextPtr += Op->BaseOffset;
            ContextPtr += Index * Op->Stride;

            #define LOAD_CTX(x, y) \
              case x: { \
                y const *Data = reinterpret_cast<y const*>(ContextPtr); \
                GD = *Data; \
                break; \
              }
            switch (Op->Size) {
              LOAD_CTX(1, uint8_t)
              LOAD_CTX(2, uint16_t)
              LOAD_CTX(4, uint32_t)
              LOAD_CTX(8, uint64_t)
              case 16: {
                void const *Data = reinterpret_cast<void const*>(ContextPtr);
                memcpy(GDP, Data, Op->Size);
                break;
              }
              default:  LogMan::Msg::A("Unhandled LoadContextIndexed size: %d", Op->Size);
            }
            #undef LOAD_CTX
            break;
          }
          case IR::OP_STORECONTEXT: {
            auto Op = IROp->C<IR::IROp_StoreContext>();

            uintptr_t ContextPtr = reinterpret_cast<uintptr_t>(Thread->CurrentFrame);
            ContextPtr += Op->Offset;

            void *Data = reinterpret_cast<void*>(ContextPtr);
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            memcpy(Data, Src, OpSize);
            break;
          }
          case IR::OP_STORECONTEXTINDEXED: {
            auto Op = IROp->C<IR::IROp_StoreContextIndexed>();
            uint64_t Index = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);

            uintptr_t ContextPtr = reinterpret_cast<uintptr_t>(Thread->CurrentFrame);
            ContextPtr += Op->BaseOffset;
            ContextPtr += Index * Op->Stride;

            void *Data = reinterpret_cast<void*>(ContextPtr);
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            memcpy(Data, Src, Op->Size);
            break;
          }
          case IR::OP_CREATEELEMENTPAIR: {
            auto Op = IROp->C<IR::IROp_CreateElementPair>();
            void *Src_Lower = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src_Upper = GetSrc<void*>(SSAData, Op->Header.Args[1]);

            uint8_t *Dst = GetDest<uint8_t*>(SSAData, WrapperOp);

            memcpy(Dst, Src_Lower, Op->Header.Size);
            memcpy(Dst + Op->Header.Size, Src_Upper, Op->Header.Size);
            break;
          }
          case IR::OP_EXTRACTELEMENTPAIR: {
            auto Op = IROp->C<IR::IROp_ExtractElementPair>();
            uintptr_t Src = GetSrc<uintptr_t>(SSAData, Op->Header.Args[0]);
            memcpy(GDP,
              reinterpret_cast<void*>(Src + Op->Header.Size * Op->Element), Op->Header.Size);
            break;
          }
          case IR::OP_CASPAIR: {
            auto Op = IROp->C<IR::IROp_CASPair>();
            auto Size = OpSize;
            // Size is the size of each pair element
            switch (Size) {
              case 4: {
                std::atomic<uint64_t> *Data = *GetSrc<std::atomic<uint64_t> **>(SSAData, Op->Header.Args[2]);

                uint64_t Src1 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
                uint64_t Src2 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);

                uint64_t Expected = Src1;
                bool Result = Data->compare_exchange_strong(Expected, Src2);
                GD = Result ? Src1 : Expected;
                break;
              }
              case 8: {
                std::atomic<__uint128_t> *Data = *GetSrc<std::atomic<__uint128_t> **>(SSAData, Op->Header.Args[2]);

                __uint128_t Src1 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]);
                __uint128_t Src2 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[1]);

                __uint128_t Expected = Src1;
                bool Result = Data->compare_exchange_strong(Expected, Src2);
                memcpy(GDP, Result ? &Src1 : &Expected, 16);
                break;
              }
              default: LogMan::Msg::A("Unknown CAS size: %d", Size); break;
            }
            break;
          }
          case IR::OP_TRUNCELEMENTPAIR: {
            auto Op = IROp->C<IR::IROp_TruncElementPair>();

            switch (Op->Size) {
              case 4: {
                uint64_t *Src = GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
                uint64_t Result{};
                Result = Src[0] & ~0U;
                Result |= Src[1] << 32;
                GD = Result;
                break;
              }
              default: LogMan::Msg::A("Unhandled Truncation size: %d", Op->Size); break;
            }
            break;
          }
          case IR::OP_LOADFLAG: {
            auto Op = IROp->C<IR::IROp_LoadFlag>();

            uintptr_t ContextPtr = reinterpret_cast<uintptr_t>(Thread->CurrentFrame);
            ContextPtr += offsetof(FEXCore::Core::CPUState, flags[0]);
            ContextPtr += Op->Flag;
            uint8_t const *Data = reinterpret_cast<uint8_t const*>(ContextPtr);
            GD = *Data;
            break;
          }
          case IR::OP_STOREFLAG: {
            auto Op = IROp->C<IR::IROp_StoreFlag>();
            uint8_t Arg = *GetSrc<uint8_t*>(SSAData, Op->Header.Args[0]);

            uintptr_t ContextPtr = reinterpret_cast<uintptr_t>(Thread->CurrentFrame);
            ContextPtr += offsetof(FEXCore::Core::CPUState, flags[0]);
            ContextPtr += Op->Flag;
            uint8_t *Data = reinterpret_cast<uint8_t*>(ContextPtr);
            *Data = Arg;
            break;
          }
          case IR::OP_LOADMEM:
          case IR::OP_LOADMEMTSO: {
            auto Op = IROp->C<IR::IROp_LoadMem>();
            uint8_t const *Data = *GetSrc<uint8_t const**>(SSAData, Op->Addr);

            if (!Op->Offset.IsInvalid()) {
              auto Offset = *GetSrc<uintptr_t const*>(SSAData, Op->Offset) * Op->OffsetScale;

              switch(Op->OffsetType.Val) {
                case MEM_OFFSET_SXTX.Val: Data +=  Offset; break;
                case MEM_OFFSET_UXTW.Val: Data += (uint32_t)Offset; break;
                case MEM_OFFSET_SXTW.Val: Data += (int32_t)Offset; break;
              }
            }
            memset(GDP, 0, 16);
            memcpy(GDP, Data, Op->Size);
            break;
          }
          case IR::OP_VLOADMEMELEMENT: {
            auto Op = IROp->C<IR::IROp_VLoadMemElement>();
            void const *Data = *GetSrc<void const**>(SSAData, Op->Header.Args[0]);

            memcpy(GDP, GetSrc<void*>(SSAData, Op->Header.Args[1]), 16);
            memcpy(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(GDP) + (Op->Header.ElementSize * Op->Index)),
              Data, Op->Header.ElementSize);
            break;
          }
          case IR::OP_STOREMEM:
          case IR::OP_STOREMEMTSO: {
            auto Op = IROp->C<IR::IROp_StoreMem>();

            uint8_t *Data = *GetSrc<uint8_t **>(SSAData, Op->Addr);

            if (!Op->Offset.IsInvalid()) {
              auto Offset = *GetSrc<uintptr_t const*>(SSAData, Op->Offset) * Op->OffsetScale;

              switch(Op->OffsetType.Val) {
                case MEM_OFFSET_SXTX.Val: Data +=  Offset; break;
                case MEM_OFFSET_UXTW.Val: Data += (uint32_t)Offset; break;
                case MEM_OFFSET_SXTW.Val: Data += (int32_t)Offset; break;
              }
            }
            memcpy(Data, GetSrc<void*>(SSAData, Op->Value), Op->Size);
            break;
          }
          case IR::OP_VSTOREMEMELEMENT: {
            #define STORE_DATA(x, y) \
              case x: { \
                y *Data = *GetSrc<y**>(SSAData, Op->Header.Args[0]); \
                memcpy(Data, &GetSrc<y*>(SSAData, Op->Header.Args[1])[Op->Index], sizeof(y)); \
                break; \
              }

            auto Op = IROp->C<IR::IROp_VStoreMemElement>();

            switch (OpSize) {
              STORE_DATA(1, uint8_t)
              STORE_DATA(2, uint16_t)
              STORE_DATA(4, uint32_t)
              STORE_DATA(8, uint64_t)
              default: LogMan::Msg::A("Unhandled StoreMem size"); break;
            }
            #undef STORE_DATA
            break;
          }
          #define DO_OP(size, type, func)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(GDP);  \
            auto *Src1_d = reinterpret_cast<type*>(Src1); \
            auto *Src2_d = reinterpret_cast<type*>(Src2); \
            *Dst_d = func(*Src1_d, *Src2_d);          \
            break;                                            \
            }

          case IR::OP_ADD: {
            auto Op = IROp->C<IR::IROp_Add>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            auto Func = [](auto a, auto b) { return a + b; };

            switch (OpSize) {
              DO_OP(4, uint32_t, Func)
              DO_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_SUB: {
            auto Op = IROp->C<IR::IROp_Sub>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            auto Func = [](auto a, auto b) { return a - b; };

            switch (OpSize) {
              DO_OP(4, uint32_t, Func)
              DO_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_NEG: {
            auto Op = IROp->C<IR::IROp_Neg>();
            uint64_t Src = *GetSrc<int64_t*>(SSAData, Op->Header.Args[0]);
            switch (OpSize) {
              case 4:
                GD = -static_cast<int32_t>(Src);
                break;
              case 8:
                GD = -static_cast<int64_t>(Src);
                break;
              default: LogMan::Msg::A("Unknown NEG Size: %d\n", OpSize); break;
            };
            break;
          }
          case IR::OP_OR: {
            auto Op = IROp->C<IR::IROp_Or>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            auto Func = [](auto a, auto b) { return a | b; };

            switch (OpSize) {
              DO_OP(1, uint8_t,  Func)
              DO_OP(2, uint16_t, Func)
              DO_OP(4, uint32_t, Func)
              DO_OP(8, uint64_t, Func)
              DO_OP(16, __uint128_t, Func)
              default: LogMan::Msg::A("Unknown Size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_AND: {
            auto Op = IROp->C<IR::IROp_And>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            auto Func = [](auto a, auto b) { return a & b; };

            switch (OpSize) {
              DO_OP(1, uint8_t,  Func)
              DO_OP(2, uint16_t, Func)
              DO_OP(4, uint32_t, Func)
              DO_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_XOR: {
            auto Op = IROp->C<IR::IROp_Xor>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            auto Func = [](auto a, auto b) { return a ^ b; };

            switch (OpSize) {
              DO_OP(1, uint8_t,  Func)
              DO_OP(2, uint16_t, Func)
              DO_OP(4, uint32_t, Func)
              DO_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_LSHL: {
            auto Op = IROp->C<IR::IROp_Lshl>();
            uint64_t Src1 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
            uint8_t Mask = OpSize * 8 - 1;
            switch (OpSize) {
              case 4:
                GD = static_cast<int32_t>(Src1) << (Src2 & Mask);
                break;
              case 8:
                GD = static_cast<int64_t>(Src1) << (Src2 & Mask);
                break;
              default: LogMan::Msg::A("Unknown LSHL Size: %d\n", OpSize); break;
            };
            break;
          }
          case IR::OP_LSHR: {
            auto Op = IROp->C<IR::IROp_Lshr>();
            uint64_t Src1 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
            uint8_t Mask = OpSize * 8 - 1;
            switch (OpSize) {
              case 4:
                GD = static_cast<uint32_t>(Src1) >> (Src2 & Mask);
                break;
              case 8:
                GD = static_cast<uint64_t>(Src1) >> (Src2 & Mask);
                break;
              default: LogMan::Msg::A("Unknown LSHR Size: %d\n", OpSize); break;
            };
            break;
          }
          case IR::OP_ASHR: {
            auto Op = IROp->C<IR::IROp_Ashr>();
            uint64_t Src1 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
            uint8_t Mask = OpSize * 8 - 1;
            switch (OpSize) {
              case 4:
                GD = (uint32_t)(static_cast<int32_t>(Src1) >> (Src2 & Mask));
                break;
              case 8:
                GD = (uint64_t)(static_cast<int64_t>(Src1) >> (Src2 & Mask));
                break;
              default: LogMan::Msg::A("Unknown ASHR Size: %d\n", OpSize); break;
            };
            break;
          }
          case IR::OP_ROR: {
            auto Op = IROp->C<IR::IROp_Ror>();
            uint64_t Src1 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
            auto Ror = [] (auto In, auto R) {
            auto RotateMask = sizeof(In) * 8 - 1;
              R &= RotateMask;
              return (In >> R) | (In << (sizeof(In) * 8 - R));
            };

            switch (OpSize) {
              case 4:
                GD = Ror(static_cast<uint32_t>(Src1), static_cast<uint32_t>(Src2));
                break;
              case 8: {
                GD = Ror(static_cast<uint64_t>(Src1), static_cast<uint64_t>(Src2));
                break;
              }
              default: LogMan::Msg::A("Unknown ROR Size: %d\n", OpSize); break;
            }
            break;
          }
          case IR::OP_EXTR: {
            auto Op = IROp->C<IR::IROp_Extr>();
            uint64_t Src1 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
            auto Extr = [] (auto Src1, auto Src2, uint8_t lsb) -> decltype(Src1) {
              __uint128_t Result{};
              Result = Src1;
              Result <<= sizeof(Src1) * 8;
              Result |= Src2;
              Result >>= lsb;
              return Result;
            };

            switch (OpSize) {
              case 4:
                GD = Extr(static_cast<uint32_t>(Src1), static_cast<uint32_t>(Src2), Op->LSB);
                break;
              case 8: {
                GD = Extr(static_cast<uint64_t>(Src1), static_cast<uint64_t>(Src2), Op->LSB);
                break;
              }
              default: LogMan::Msg::A("Unknown EXTR Size: %d\n", OpSize); break;
            }
            break;
          }
          case IR::OP_NOT: {
            auto Op = IROp->C<IR::IROp_Not>();
            uint64_t Src = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
            const uint64_t mask[9]= { 0, 0xFF, 0xFFFF, 0, 0xFFFFFFFF, 0, 0, 0, 0xFFFFFFFFFFFFFFFFULL };
            uint64_t Mask = mask[OpSize];
            GD = (~Src) & Mask;
            break;
          }
          case IR::OP_MUL: {
            auto Op = IROp->C<IR::IROp_Mul>();
            uint64_t Src1 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);

            switch (OpSize) {
              case 4:
                GD = static_cast<int64_t>(static_cast<int32_t>(Src1)) * static_cast<int64_t>(static_cast<int32_t>(Src2));
                break;
              case 8:
                GD = static_cast<int64_t>(Src1) * static_cast<int64_t>(Src2);
                break;
              case 16: {
                __int128_t Tmp = static_cast<__int128_t>(static_cast<int64_t>(Src1)) * static_cast<__int128_t>(static_cast<int64_t>(Src2));
                memcpy(GDP, &Tmp, 16);
                break;
              }
              default: LogMan::Msg::A("Unknown Mul Size: %d\n", OpSize); break;
            }
            break;
          }
          case IR::OP_MULH: {
            auto Op = IROp->C<IR::IROp_MulH>();
            uint64_t Src1 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);

            switch (OpSize) {
              case 4: {
                int64_t Tmp = static_cast<int64_t>(static_cast<int32_t>(Src1)) * static_cast<int64_t>(static_cast<int32_t>(Src2));
                GD = Tmp >> 32;
                break;
              }
              case 8: {
                __int128_t Tmp = static_cast<__int128_t>(static_cast<int64_t>(Src1)) * static_cast<__int128_t>(static_cast<int64_t>(Src2));
                GD = Tmp >> 64;
              }
              break;
              default: LogMan::Msg::A("Unknown MulH Size: %d\n", OpSize); break;
            }
            break;
          }
          case IR::OP_UMUL: {
            auto Op = IROp->C<IR::IROp_UMul>();
            uint64_t Src1 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);

            switch (OpSize) {
              case 4:
                GD = static_cast<uint32_t>(Src1) * static_cast<uint32_t>(Src2);
                break;
              case 8:
                GD = static_cast<uint64_t>(Src1) * static_cast<uint64_t>(Src2);
                break;
              case 16: {
                __uint128_t Tmp = static_cast<__uint128_t>(static_cast<uint64_t>(Src1)) * static_cast<__uint128_t>(static_cast<uint64_t>(Src2));
                memcpy(GDP, &Tmp, 16);
                break;
              }
              default: LogMan::Msg::A("Unknown UMul Size: %d\n", OpSize); break;
            }
            break;
          }
          case IR::OP_UMULH: {
            auto Op = IROp->C<IR::IROp_UMulH>();
            uint64_t Src1 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
            switch (OpSize) {
              case 4:
                GD = static_cast<uint64_t>(Src1) * static_cast<uint64_t>(Src2);
                GD >>= 32;
                break;
              case 8: {
                __uint128_t Tmp = static_cast<__uint128_t>(Src1) * static_cast<__uint128_t>(Src2);
                GD = Tmp >> 64;
                break;
              }
              case 16: {
                // XXX: This is incorrect
                __uint128_t Tmp = static_cast<__uint128_t>(Src1) * static_cast<__uint128_t>(Src2);
                GD = Tmp >> 64;
                break;
              }
              default: LogMan::Msg::A("Unknown UMulH Size: %d\n", OpSize); break;
            }
            break;
          }
          case IR::OP_DIV: {
            auto Op = IROp->C<IR::IROp_Div>();
            uint64_t Src1 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);

            switch (OpSize) {
              case 1:
                GD = static_cast<int64_t>(static_cast<int8_t>(Src1)) / static_cast<int64_t>(static_cast<int8_t>(Src2));
                break;
              case 2:
                GD = static_cast<int64_t>(static_cast<int16_t>(Src1)) / static_cast<int64_t>(static_cast<int16_t>(Src2));
                break;
              case 4:
                GD = static_cast<int64_t>(static_cast<int32_t>(Src1)) / static_cast<int64_t>(static_cast<int32_t>(Src2));
                break;
              case 8:
                GD = static_cast<int64_t>(Src1) / static_cast<int64_t>(Src2);
                break;
              case 16: {
                __int128_t Tmp = *GetSrc<__int128_t*>(SSAData, Op->Header.Args[0]) / *GetSrc<__int128_t*>(SSAData, Op->Header.Args[1]);
                memcpy(GDP, &Tmp, 16);
                break;
              }
              default: LogMan::Msg::A("Unknown Mul Size: %d\n", OpSize); break;
            }
            break;
          }
          case IR::OP_UDIV: {
            auto Op = IROp->C<IR::IROp_UDiv>();
            uint64_t Src1 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);

            switch (OpSize) {
              case 1:
                GD = static_cast<uint64_t>(static_cast<uint8_t>(Src1)) / static_cast<uint64_t>(static_cast<uint8_t>(Src2));
                break;
              case 2:
                GD = static_cast<uint64_t>(static_cast<uint16_t>(Src1)) / static_cast<uint64_t>(static_cast<uint16_t>(Src2));
                break;
              case 4:
                GD = static_cast<uint64_t>(static_cast<uint32_t>(Src1)) / static_cast<uint64_t>(static_cast<uint32_t>(Src2));
                break;
              case 8:
                GD = static_cast<uint64_t>(Src1) / static_cast<uint64_t>(Src2);
                break;
              case 16: {
                __uint128_t Tmp = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]) / *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[1]);
                memcpy(GDP, &Tmp, 16);
                break;
              }
              default: LogMan::Msg::A("Unknown Mul Size: %d\n", OpSize); break;
            }
            break;
          }
          case IR::OP_REM: {
            auto Op = IROp->C<IR::IROp_Rem>();
            uint64_t Src1 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);

            switch (OpSize) {
              case 1:
                GD = static_cast<int64_t>(static_cast<int8_t>(Src1)) % static_cast<int64_t>(static_cast<int8_t>(Src2));
                break;
              case 2:
                GD = static_cast<int64_t>(static_cast<int16_t>(Src1)) % static_cast<int64_t>(static_cast<int16_t>(Src2));
                break;
              case 4:
                GD = static_cast<int64_t>(static_cast<int32_t>(Src1)) % static_cast<int64_t>(static_cast<int32_t>(Src2));
                break;
              case 8:
                GD = static_cast<int64_t>(Src1) % static_cast<int64_t>(Src2);
                break;
              case 16: {
                __int128_t Tmp = *GetSrc<__int128_t*>(SSAData, Op->Header.Args[0]) % *GetSrc<__int128_t*>(SSAData, Op->Header.Args[1]);
                memcpy(GDP, &Tmp, 16);
                break;
              }
              default: LogMan::Msg::A("Unknown Mul Size: %d\n", OpSize); break;
            }
            break;
          }
          case IR::OP_UREM: {
            auto Op = IROp->C<IR::IROp_URem>();
            uint64_t Src1 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);

            switch (OpSize) {
              case 1:
                GD = static_cast<uint64_t>(static_cast<uint8_t>(Src1)) % static_cast<uint64_t>(static_cast<uint8_t>(Src2));
                break;
              case 2:
                GD = static_cast<uint64_t>(static_cast<uint16_t>(Src1)) % static_cast<uint64_t>(static_cast<uint16_t>(Src2));
                break;
              case 4:
                GD = static_cast<uint64_t>(static_cast<uint32_t>(Src1)) % static_cast<uint64_t>(static_cast<uint32_t>(Src2));
                break;
              case 8:
                GD = static_cast<uint64_t>(Src1) % static_cast<uint64_t>(Src2);
                break;
              case 16: {
                __uint128_t Tmp = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]) % *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[1]);
                memcpy(GDP, &Tmp, 16);
                break;
              }
              default: LogMan::Msg::A("Unknown Mul Size: %d\n", OpSize); break;
            }
            break;
          }
          case IR::OP_POPCOUNT: {
            auto Op = IROp->C<IR::IROp_Popcount>();
            uint64_t Src = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
            GD = __builtin_popcountl(Src);
            break;
          }
          case IR::OP_FINDLSB: {
            auto Op = IROp->C<IR::IROp_FindLSB>();
            uint64_t Src = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
            uint64_t Result = __builtin_ffsll(Src);
            GD = Result - 1;
            break;
          }
          case IR::OP_FINDMSB: {
            auto Op = IROp->C<IR::IROp_FindMSB>();
            switch (OpSize) {
              case 1: GD = ((24 + OpSize * 8) - __builtin_clz(*GetSrc<uint8_t*>(SSAData, Op->Header.Args[0]))) - 1; break;
              case 2: GD = ((16 + OpSize * 8) - __builtin_clz(*GetSrc<uint16_t*>(SSAData, Op->Header.Args[0]))) - 1; break;
              case 4: GD = (OpSize * 8 - __builtin_clz(*GetSrc<uint32_t*>(SSAData, Op->Header.Args[0]))) - 1; break;
              case 8: GD = (OpSize * 8 - __builtin_clzll(*GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]))) - 1; break;
              default: LogMan::Msg::A("Unknown REV size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_REV: {
            auto Op = IROp->C<IR::IROp_Rev>();
            switch (OpSize) {
              case 2: GD = __builtin_bswap16(*GetSrc<uint16_t*>(SSAData, Op->Header.Args[0])); break;
              case 4: GD = __builtin_bswap32(*GetSrc<uint32_t*>(SSAData, Op->Header.Args[0])); break;
              case 8: GD = __builtin_bswap64(*GetSrc<uint64_t*>(SSAData, Op->Header.Args[0])); break;
              default: LogMan::Msg::A("Unknown REV size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_FINDTRAILINGZEROS: {
            auto Op = IROp->C<IR::IROp_FindTrailingZeros>();
            switch (OpSize) {
              case 1: {
                auto Src = *GetSrc<uint8_t*>(SSAData, Op->Header.Args[0]);
                if (Src)
                  GD = __builtin_ctz(Src);
                else
                  GD = sizeof(Src) * 8;
                break;
              }
              case 2: {
                auto Src = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[0]);
                if (Src)
                  GD = __builtin_ctz(Src);
                else
                  GD = sizeof(Src) * 8;
                break;
              }
              case 4: {
                auto Src = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[0]);
                if (Src)
                  GD = __builtin_ctz(Src);
                else
                  GD = sizeof(Src) * 8;
                break;
              }
              case 8: {
                auto Src = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
                if (Src)
                  GD = __builtin_ctzll(Src);
                else
                  GD = sizeof(Src) * 8;
                break;
              }
              default: LogMan::Msg::A("Unknown size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_COUNTLEADINGZEROES: {
            auto Op = IROp->C<IR::IROp_CountLeadingZeroes>();
            switch (OpSize) {
              case 1: {
                uint32_t Src = *GetSrc<uint8_t*>(SSAData, Op->Header.Args[0]);
                Src <<= 24;
                if (Src)
                  GD = __builtin_clz(Src);
                else
                  GD = 8;
                break;
              }
              case 2: {
                uint32_t Src = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[0]);
                Src <<= 16;
                if (Src)
                  GD = __builtin_clz(Src);
                else
                  GD = 16;
                break;
              }
              case 4: {
                auto Src = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[0]);
                if (Src)
                  GD = __builtin_clz(Src);
                else
                  GD = sizeof(Src) * 8;
                break;
              }
              case 8: {
                auto Src = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
                if (Src)
                  GD = __builtin_clzll(Src);
                else
                  GD = sizeof(Src) * 8;
                break;
              }
              default: LogMan::Msg::A("Unknown size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_BFI: {
            auto Op = IROp->C<IR::IROp_Bfi>();
            uint64_t SourceMask = (1ULL << Op->Width) - 1;
            if (Op->Width == 64)
              SourceMask = ~0ULL;
            uint64_t DestMask = ~(SourceMask << Op->lsb);
            uint64_t Src1 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
            uint64_t Res = (Src1 & DestMask) | ((Src2 & SourceMask) << Op->lsb);
            GD = Res;
            break;
          }
          case IR::OP_SBFE: {
            auto Op = IROp->C<IR::IROp_Sbfe>();
            LogMan::Throw::A(OpSize < 16, "OpSize is too large for BFE: %d", OpSize);
            int64_t Src = *GetSrc<int64_t*>(SSAData, Op->Header.Args[0]);
            uint64_t ShiftLeftAmount = (64 - (Op->Width + Op->lsb));
            uint64_t ShiftRightAmount = ShiftLeftAmount + Op->lsb;
            Src <<= ShiftLeftAmount;
            Src >>= ShiftRightAmount;
            GD = Src;
            break;
          }
          case IR::OP_BFE: {
            auto Op = IROp->C<IR::IROp_Bfe>();
            LogMan::Throw::A(OpSize <= 8, "OpSize is too large for BFE: %d", OpSize);
            uint64_t SourceMask = (1ULL << Op->Width) - 1;
            if (Op->Width == 64)
              SourceMask = ~0ULL;
            SourceMask <<= Op->lsb;
            uint64_t Src = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
            GD = (Src & SourceMask) >> Op->lsb;
            break;
          }
          case IR::OP_SELECT: {
            auto Op = IROp->C<IR::IROp_Select>();

            uint64_t Src1 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);

            uint64_t ArgTrue;
            uint64_t ArgFalse;

            if (OpSize == 4) {
              ArgTrue = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[2]);
              ArgFalse = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[3]);
            } else {
              ArgTrue = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[2]);
              ArgFalse = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[3]);
            }

            bool CompResult;

            if (Op->CompareSize == 4)
              CompResult = IsConditionTrue<uint32_t, int32_t, float>(Op->Cond.Val, Src1, Src2);
            else
              CompResult = IsConditionTrue<uint64_t, int64_t, double>(Op->Cond.Val, Src1, Src2);

            GD = CompResult ? ArgTrue : ArgFalse;
            break;
          }
          case IR::OP_CAS: {
            auto Op = IROp->C<IR::IROp_CAS>();
            auto Size = OpSize;
            switch (Size) {
              case 1: {
                std::atomic<uint8_t> *Data = *GetSrc<std::atomic<uint8_t> **>(SSAData, Op->Header.Args[2]);

                uint8_t Src1 = *GetSrc<uint8_t*>(SSAData, Op->Header.Args[0]);
                uint8_t Src2 = *GetSrc<uint8_t*>(SSAData, Op->Header.Args[1]);

                uint8_t Expected = Src1;
                bool Result = Data->compare_exchange_strong(Expected, Src2);
                GD = Result ? Src1 : Expected;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = *GetSrc<std::atomic<uint16_t> **>(SSAData, Op->Header.Args[2]);
                uint16_t Src1 = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[0]);
                uint16_t Src2 = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[1]);

                uint16_t Expected = Src1;
                bool Result = Data->compare_exchange_strong(Expected, Src2);
                GD = Result ? Src1 : Expected;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = *GetSrc<std::atomic<uint32_t> **>(SSAData, Op->Header.Args[2]);

                uint32_t Src1 = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[0]);
                uint32_t Src2 = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[1]);

                uint32_t Expected = Src1;
                bool Result = Data->compare_exchange_strong(Expected, Src2);
                GD = Result ? Src1 : Expected;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = *GetSrc<std::atomic<uint64_t> **>(SSAData, Op->Header.Args[2]);

                uint64_t Src1 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
                uint64_t Src2 = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);

                uint64_t Expected = Src1;
                bool Result = Data->compare_exchange_strong(Expected, Src2);
                GD = Result ? Src1 : Expected;
                break;
              }
              default: LogMan::Msg::A("Unknown CAS size: %d", Size); break;
            }
            break;
          }
          case IR::OP_ATOMICADD: {
            auto Op = IROp->C<IR::IROp_AtomicAdd>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = *GetSrc<std::atomic<uint8_t> **>(SSAData, Op->Header.Args[0]);
                uint8_t Src = *GetSrc<uint8_t*>(SSAData, Op->Header.Args[1]);
                *Data += Src;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = *GetSrc<std::atomic<uint16_t> **>(SSAData, Op->Header.Args[0]);
                uint16_t Src = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[1]);
                *Data += Src;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = *GetSrc<std::atomic<uint32_t> **>(SSAData, Op->Header.Args[0]);
                uint32_t Src = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[1]);
                *Data += Src;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = *GetSrc<std::atomic<uint64_t> **>(SSAData, Op->Header.Args[0]);
                uint64_t Src = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
                *Data += Src;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          case IR::OP_ATOMICSUB: {
            auto Op = IROp->C<IR::IROp_AtomicSub>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = *GetSrc<std::atomic<uint8_t> **>(SSAData, Op->Header.Args[0]);
                uint8_t Src = *GetSrc<uint8_t*>(SSAData, Op->Header.Args[1]);
                *Data -= Src;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = *GetSrc<std::atomic<uint16_t> **>(SSAData, Op->Header.Args[0]);
                uint16_t Src = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[1]);
                *Data -= Src;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = *GetSrc<std::atomic<uint32_t> **>(SSAData, Op->Header.Args[0]);
                uint32_t Src = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[1]);
                *Data -= Src;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = *GetSrc<std::atomic<uint64_t> **>(SSAData, Op->Header.Args[0]);
                uint64_t Src = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
                *Data -= Src;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          case IR::OP_ATOMICAND: {
            auto Op = IROp->C<IR::IROp_AtomicAnd>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = *GetSrc<std::atomic<uint8_t> **>(SSAData, Op->Header.Args[0]);
                uint8_t Src = *GetSrc<uint8_t*>(SSAData, Op->Header.Args[1]);
                *Data &= Src;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = *GetSrc<std::atomic<uint16_t> **>(SSAData, Op->Header.Args[0]);
                uint16_t Src = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[1]);
                *Data &= Src;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = *GetSrc<std::atomic<uint32_t> **>(SSAData, Op->Header.Args[0]);
                uint32_t Src = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[1]);
                *Data &= Src;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = *GetSrc<std::atomic<uint64_t> **>(SSAData, Op->Header.Args[0]);
                uint64_t Src = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
                *Data &= Src;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          case IR::OP_ATOMICOR: {
            auto Op = IROp->C<IR::IROp_AtomicOr>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = *GetSrc<std::atomic<uint8_t> **>(SSAData, Op->Header.Args[0]);
                uint8_t Src = *GetSrc<uint8_t*>(SSAData, Op->Header.Args[1]);
                *Data |= Src;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = *GetSrc<std::atomic<uint16_t> **>(SSAData, Op->Header.Args[0]);
                uint16_t Src = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[1]);
                *Data |= Src;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = *GetSrc<std::atomic<uint32_t> **>(SSAData, Op->Header.Args[0]);
                uint32_t Src = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[1]);
                *Data |= Src;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = *GetSrc<std::atomic<uint64_t> **>(SSAData, Op->Header.Args[0]);
                uint64_t Src = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
                *Data |= Src;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          case IR::OP_ATOMICXOR: {
            auto Op = IROp->C<IR::IROp_AtomicXor>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = *GetSrc<std::atomic<uint8_t> **>(SSAData, Op->Header.Args[0]);
                uint8_t Src = *GetSrc<uint8_t*>(SSAData, Op->Header.Args[1]);
                *Data ^= Src;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = *GetSrc<std::atomic<uint16_t> **>(SSAData, Op->Header.Args[0]);
                uint16_t Src = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[1]);
                *Data ^= Src;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = *GetSrc<std::atomic<uint32_t> **>(SSAData, Op->Header.Args[0]);
                uint32_t Src = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[1]);
                *Data ^= Src;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = *GetSrc<std::atomic<uint64_t> **>(SSAData, Op->Header.Args[0]);
                uint64_t Src = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
                *Data ^= Src;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          case IR::OP_ATOMICSWAP: {
            auto Op = IROp->C<IR::IROp_AtomicSwap>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = *GetSrc<std::atomic<uint8_t> **>(SSAData, Op->Header.Args[0]);
                uint8_t Src = *GetSrc<uint8_t*>(SSAData, Op->Header.Args[1]);
                uint8_t Previous = Data->exchange(Src);
                GD = Previous;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = *GetSrc<std::atomic<uint16_t> **>(SSAData, Op->Header.Args[0]);
                uint16_t Src = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[1]);
                uint16_t Previous = Data->exchange(Src);
                GD = Previous;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = *GetSrc<std::atomic<uint32_t> **>(SSAData, Op->Header.Args[0]);
                uint32_t Src = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[1]);
                uint32_t Previous = Data->exchange(Src);
                GD = Previous;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = *GetSrc<std::atomic<uint64_t> **>(SSAData, Op->Header.Args[0]);
                uint64_t Src = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
                uint64_t Previous = Data->exchange(Src);
                GD = Previous;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          case IR::OP_ATOMICFETCHADD: {
            auto Op = IROp->C<IR::IROp_AtomicFetchAdd>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = *GetSrc<std::atomic<uint8_t> **>(SSAData, Op->Header.Args[0]);
                uint8_t Src = *GetSrc<uint8_t*>(SSAData, Op->Header.Args[1]);
                uint8_t Previous = Data->fetch_add(Src);
                GD = Previous;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = *GetSrc<std::atomic<uint16_t> **>(SSAData, Op->Header.Args[0]);
                uint16_t Src = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[1]);
                uint16_t Previous = Data->fetch_add(Src);
                GD = Previous;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = *GetSrc<std::atomic<uint32_t> **>(SSAData, Op->Header.Args[0]);
                uint32_t Src = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[1]);
                uint32_t Previous = Data->fetch_add(Src);
                GD = Previous;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = *GetSrc<std::atomic<uint64_t> **>(SSAData, Op->Header.Args[0]);
                uint64_t Src = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
                uint64_t Previous = Data->fetch_add(Src);
                GD = Previous;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          case IR::OP_ATOMICFETCHSUB: {
            auto Op = IROp->C<IR::IROp_AtomicFetchSub>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = *GetSrc<std::atomic<uint8_t> **>(SSAData, Op->Header.Args[0]);
                uint8_t Src = *GetSrc<uint8_t*>(SSAData, Op->Header.Args[1]);
                uint8_t Previous = Data->fetch_sub(Src);
                GD = Previous;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = *GetSrc<std::atomic<uint16_t> **>(SSAData, Op->Header.Args[0]);
                uint16_t Src = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[1]);
                uint16_t Previous = Data->fetch_sub(Src);
                GD = Previous;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = *GetSrc<std::atomic<uint32_t> **>(SSAData, Op->Header.Args[0]);
                uint32_t Src = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[1]);
                uint32_t Previous = Data->fetch_sub(Src);
                GD = Previous;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = *GetSrc<std::atomic<uint64_t> **>(SSAData, Op->Header.Args[0]);
                uint64_t Src = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
                uint64_t Previous = Data->fetch_sub(Src);
                GD = Previous;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          case IR::OP_ATOMICFETCHAND: {
            auto Op = IROp->C<IR::IROp_AtomicFetchAnd>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = *GetSrc<std::atomic<uint8_t> **>(SSAData, Op->Header.Args[0]);
                uint8_t Src = *GetSrc<uint8_t*>(SSAData, Op->Header.Args[1]);
                uint8_t Previous = Data->fetch_and(Src);
                GD = Previous;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = *GetSrc<std::atomic<uint16_t> **>(SSAData, Op->Header.Args[0]);
                uint16_t Src = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[1]);
                uint16_t Previous = Data->fetch_and(Src);
                GD = Previous;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = *GetSrc<std::atomic<uint32_t> **>(SSAData, Op->Header.Args[0]);
                uint32_t Src = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[1]);
                uint32_t Previous = Data->fetch_and(Src);
                GD = Previous;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = *GetSrc<std::atomic<uint64_t> **>(SSAData, Op->Header.Args[0]);
                uint64_t Src = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
                uint64_t Previous = Data->fetch_and(Src);
                GD = Previous;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          case IR::OP_ATOMICFETCHOR: {
            auto Op = IROp->C<IR::IROp_AtomicFetchOr>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = *GetSrc<std::atomic<uint8_t> **>(SSAData, Op->Header.Args[0]);
                uint8_t Src = *GetSrc<uint8_t*>(SSAData, Op->Header.Args[1]);
                uint8_t Previous = Data->fetch_or(Src);
                GD = Previous;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = *GetSrc<std::atomic<uint16_t> **>(SSAData, Op->Header.Args[0]);
                uint16_t Src = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[1]);
                uint16_t Previous = Data->fetch_or(Src);
                GD = Previous;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = *GetSrc<std::atomic<uint32_t> **>(SSAData, Op->Header.Args[0]);
                uint32_t Src = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[1]);
                uint32_t Previous = Data->fetch_or(Src);
                GD = Previous;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = *GetSrc<std::atomic<uint64_t> **>(SSAData, Op->Header.Args[0]);
                uint64_t Src = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
                uint64_t Previous = Data->fetch_or(Src);
                GD = Previous;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          case IR::OP_ATOMICFETCHXOR: {
            auto Op = IROp->C<IR::IROp_AtomicFetchXor>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = *GetSrc<std::atomic<uint8_t> **>(SSAData, Op->Header.Args[0]);
                uint8_t Src = *GetSrc<uint8_t*>(SSAData, Op->Header.Args[1]);
                uint8_t Previous = Data->fetch_xor(Src);
                GD = Previous;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = *GetSrc<std::atomic<uint16_t> **>(SSAData, Op->Header.Args[0]);
                uint16_t Src = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[1]);
                uint16_t Previous = Data->fetch_xor(Src);
                GD = Previous;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = *GetSrc<std::atomic<uint32_t> **>(SSAData, Op->Header.Args[0]);
                uint32_t Src = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[1]);
                uint32_t Previous = Data->fetch_xor(Src);
                GD = Previous;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = *GetSrc<std::atomic<uint64_t> **>(SSAData, Op->Header.Args[0]);
                uint64_t Src = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
                uint64_t Previous = Data->fetch_xor(Src);
                GD = Previous;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          // Vector ops
          case IR::OP_CREATEVECTOR2: {
            auto Op = IROp->C<IR::IROp_CreateVector2>();
            LogMan::Throw::A(OpSize <= 16, "Can't handle a vector of size: %d", OpSize);
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];
            uint8_t ElementSize = OpSize / 2;
            #define CREATE_VECTOR(elementsize, type) \
              case elementsize: { \
                auto *Dst_d = reinterpret_cast<type*>(Tmp); \
                auto *Src1_d = reinterpret_cast<type*>(Src1); \
                auto *Src2_d = reinterpret_cast<type*>(Src2); \
                Dst_d[0] = *Src1_d; \
                Dst_d[1] = *Src2_d; \
                break; \
              }
            switch (ElementSize) {
              CREATE_VECTOR(1, uint8_t)
              CREATE_VECTOR(2, uint16_t)
              CREATE_VECTOR(4, uint32_t)
              CREATE_VECTOR(8, uint64_t)
              default: LogMan::Msg::A("Unknown Element Size: %d", ElementSize); break;
            }
            #undef CREATE_VECTOR
            memcpy(GDP, Tmp, OpSize);

            break;
          }
          case IR::OP_SPLATVECTOR4:
          case IR::OP_SPLATVECTOR2: {
            auto Op = IROp->C<IR::IROp_SplatVector2>();
            LogMan::Throw::A(OpSize <= 16, "Can't handle a vector of size: %d", OpSize);
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Tmp[16];
            uint8_t Elements = 0;

            switch (Op->Header.Op) {
              case IR::OP_SPLATVECTOR4: Elements = 4; break;
              case IR::OP_SPLATVECTOR2: Elements = 2; break;
              default: LogMan::Msg::A("Uknown Splat size"); break;
            }

            #define CREATE_VECTOR(elementsize, type) \
              case elementsize: { \
                auto *Dst_d = reinterpret_cast<type*>(Tmp); \
                auto *Src_d = reinterpret_cast<type*>(Src); \
                for (uint8_t i = 0; i < Elements; ++i) \
                  Dst_d[i] = *Src_d;\
                break; \
              }
            uint8_t ElementSize = OpSize / Elements;
            switch (ElementSize) {
              CREATE_VECTOR(1, uint8_t)
              CREATE_VECTOR(2, uint16_t)
              CREATE_VECTOR(4, uint32_t)
              CREATE_VECTOR(8, uint64_t)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.Size); break;
            }
            #undef CREATE_VECTOR
            memcpy(GDP, Tmp, OpSize);

            break;
          }
          case IR::OP_VMOV: {
            auto Op = IROp->C<IR::IROp_VMov>();
            __uint128_t Src = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]);

            memcpy(GDP, &Src, OpSize);
            break;
          }
          case IR::OP_VOR: {
            auto Op = IROp->C<IR::IROp_VOr>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]);
            __uint128_t Src2 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[1]);

            __uint128_t Dst = Src1 | Src2;
            memcpy(GDP, &Dst, 16);
            break;
          }
          case IR::OP_VAND: {
            auto Op = IROp->C<IR::IROp_VAnd>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]);
            __uint128_t Src2 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[1]);

            __uint128_t Dst = Src1 & Src2;
            memcpy(GDP, &Dst, 16);
            break;
          }
          case IR::OP_VXOR: {
            auto Op = IROp->C<IR::IROp_VXor>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]);
            __uint128_t Src2 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[1]);

            __uint128_t Dst = Src1 ^ Src2;
            memcpy(GDP, &Dst, 16);
            break;
          }
          case IR::OP_VSLI: {
            auto Op = IROp->C<IR::IROp_VSLI>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]);
            __uint128_t Src2 = Op->ByteShift * 8;

            __uint128_t Dst = Op->ByteShift >= sizeof(__uint128_t) ? 0 : Src1 << Src2;
            memcpy(GDP, &Dst, 16);
            break;
          }
          case IR::OP_VSRI: {
            auto Op = IROp->C<IR::IROp_VSRI>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]);
            __uint128_t Src2 = Op->ByteShift * 8;

            __uint128_t Dst = Op->ByteShift >= sizeof(__uint128_t) ? 0 : Src1 >> Src2;
            memcpy(GDP, &Dst, 16);
            break;
          }
          case IR::OP_VNOT: {
            auto Op = IROp->C<IR::IROp_VNot>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]);

            __uint128_t Dst = ~Src1;
            memcpy(GDP, &Dst, 16);
            break;
          }
          #define DO_VECTOR_OP(size, type, func)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src1_d = reinterpret_cast<type*>(Src1); \
            auto *Src2_d = reinterpret_cast<type*>(Src2); \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = func(Src1_d[i], Src2_d[i]);          \
            }                                                 \
            break;                                            \
            }
          #define DO_VECTOR_PAIR_OP(size, type, func)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src1_d = reinterpret_cast<type*>(Src1); \
            auto *Src2_d = reinterpret_cast<type*>(Src2); \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = func(Src1_d[i*2], Src1_d[i*2 + 1]);          \
              Dst_d[i+Elements] = func(Src2_d[i*2], Src2_d[i*2 + 1]);          \
            }                                                 \
            break;                                            \
            }
          #define DO_VECTOR_SCALAR_OP(size, type, func)\
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src1_d = reinterpret_cast<type*>(Src1); \
            auto *Src2_d = reinterpret_cast<type*>(Src2); \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = func(Src1_d[i], *Src2_d);          \
            }                                                 \
            break;                                            \
            }
          #define DO_VECTOR_0SRC_OP(size, type, func)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = func();          \
            }                                                 \
            break;                                            \
            }
          #define DO_VECTOR_1SRC_OP(size, type, func)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src_d = reinterpret_cast<type*>(Src); \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = func(Src_d[i]);          \
            }                                                 \
            break;                                            \
            }
          #define DO_VECTOR_REDUCE_1SRC_OP(size, type, func, start_val)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src_d = reinterpret_cast<type*>(Src); \
            type begin = start_val;                           \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              begin = func(begin, Src_d[i]);          \
            }                                                 \
            Dst_d[0] = begin;                                 \
            break;                                            \
            }
          #define DO_VECTOR_SAT_OP(size, type, func, min, max)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src1_d = reinterpret_cast<type*>(Src1); \
            auto *Src2_d = reinterpret_cast<type*>(Src2); \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = func(Src1_d[i], Src2_d[i], min, max);          \
            }                                                 \
            break;                                            \
            }
          case IR::OP_VECTORIMM: {
            auto Op = IROp->C<IR::IROp_VectorImm>();
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;
            uint8_t Imm = Op->Immediate;

            auto Func = [Imm]() { return Imm; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_0SRC_OP(1, int8_t, Func)
              DO_VECTOR_0SRC_OP(2, int16_t, Func)
              DO_VECTOR_0SRC_OP(4, int32_t, Func)
              DO_VECTOR_0SRC_OP(8, int64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VNEG: {
            auto Op = IROp->C<IR::IROp_VNeg>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a) { return -a; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_OP(1, int8_t, Func)
              DO_VECTOR_1SRC_OP(2, int16_t, Func)
              DO_VECTOR_1SRC_OP(4, int32_t, Func)
              DO_VECTOR_1SRC_OP(8, int64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VFNEG: {
            auto Op = IROp->C<IR::IROp_VFNeg>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a) { return -a; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_OP(4, float, Func)
              DO_VECTOR_1SRC_OP(8, double, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VUSHRI: {
            auto Op = IROp->C<IR::IROp_VUShrI>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t BitShift = Op->BitShift;
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [BitShift](auto a) { return BitShift >= (sizeof(a) * 8) ? 0 : a >> BitShift; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_OP(1, uint8_t, Func)
              DO_VECTOR_1SRC_OP(2, uint16_t, Func)
              DO_VECTOR_1SRC_OP(4, uint32_t, Func)
              DO_VECTOR_1SRC_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VSSHRI: {
            auto Op = IROp->C<IR::IROp_VSShrI>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t BitShift = Op->BitShift;
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [BitShift](auto a) { return BitShift >= (sizeof(a) * 8) ? (a >> (sizeof(a) * 8 - 1)) : a >> BitShift; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_OP(1, int8_t, Func)
              DO_VECTOR_1SRC_OP(2, int16_t, Func)
              DO_VECTOR_1SRC_OP(4, int32_t, Func)
              DO_VECTOR_1SRC_OP(8, int64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VSHLI: {
            auto Op = IROp->C<IR::IROp_VShlI>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t BitShift = Op->BitShift;
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [BitShift](auto a) { return BitShift >= (sizeof(a) * 8) ? 0 : (a << BitShift); };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_OP(1, uint8_t, Func)
              DO_VECTOR_1SRC_OP(2, uint16_t, Func)
              DO_VECTOR_1SRC_OP(4, uint32_t, Func)
              DO_VECTOR_1SRC_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }

          case IR::OP_VADD: {
            auto Op = IROp->C<IR::IROp_VAdd>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto b) { return a + b; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, uint8_t,  Func)
              DO_VECTOR_OP(2, uint16_t, Func)
              DO_VECTOR_OP(4, uint32_t, Func)
              DO_VECTOR_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VSUB: {
            auto Op = IROp->C<IR::IROp_VSub>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto b) { return a - b; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, uint8_t,  Func)
              DO_VECTOR_OP(2, uint16_t, Func)
              DO_VECTOR_OP(4, uint32_t, Func)
              DO_VECTOR_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VUQADD: {
            auto Op = IROp->C<IR::IROp_VUQAdd>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto b) {
              decltype(a) res = a + b;
              return res < a ? ~0U : res;
            };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, uint8_t,  Func)
              DO_VECTOR_OP(2, uint16_t, Func)
              DO_VECTOR_OP(4, uint32_t, Func)
              DO_VECTOR_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VUQSUB: {
            auto Op = IROp->C<IR::IROp_VUQSub>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto b) {
              decltype(a) res = a - b;
              return res > a ? 0U : res;
            };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, uint8_t,  Func)
              DO_VECTOR_OP(2, uint16_t, Func)
              DO_VECTOR_OP(4, uint32_t, Func)
              DO_VECTOR_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VSQADD: {
            auto Op = IROp->C<IR::IROp_VSQAdd>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto b) {
              decltype(a) res = a + b;

              if (a > 0) {
                if (b > (std::numeric_limits<decltype(a)>::max() - a)) {
                  return std::numeric_limits<decltype(a)>::max();
                }
              }
              else if (b < (std::numeric_limits<decltype(a)>::min() - a)) {
                return std::numeric_limits<decltype(a)>::min();
              }

              return res;
            };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, int8_t,  Func)
              DO_VECTOR_OP(2, int16_t, Func)
              DO_VECTOR_OP(4, int32_t, Func)
              DO_VECTOR_OP(8, int64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VSQSUB: {
            auto Op = IROp->C<IR::IROp_VSQSub>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto b) {
              __int128_t res = a - b;
              if (res < std::numeric_limits<decltype(a)>::min())
                return std::numeric_limits<decltype(a)>::min();

              if (res > std::numeric_limits<decltype(a)>::max())
                return std::numeric_limits<decltype(a)>::max();
              return (decltype(a))res;
            };

            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, int8_t,  Func)
              DO_VECTOR_OP(2, int16_t, Func)
              DO_VECTOR_OP(4, int32_t, Func)
              DO_VECTOR_OP(8, int64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }

          case IR::OP_VFADD: {
            auto Op = IROp->C<IR::IROp_VFAdd>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto b) { return a + b; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(4, float, Func)
              DO_VECTOR_OP(8, double, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VFADDP: {
            auto Op = IROp->C<IR::IROp_VFAddP>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = (OpSize / Op->Header.ElementSize) / 2;

            auto Func = [](auto a, auto b) { return a + b; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_PAIR_OP(4, float, Func)
              DO_VECTOR_PAIR_OP(8, double, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VFSUB: {
            auto Op = IROp->C<IR::IROp_VFSub>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto b) { return a - b; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(4, float, Func)
              DO_VECTOR_OP(8, double, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VADDP: {
            auto Op = IROp->C<IR::IROp_VAddP>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = (OpSize / Op->Header.ElementSize) / 2;

            auto Func = [](auto a, auto b) { return a + b; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_PAIR_OP(1, uint8_t,  Func)
              DO_VECTOR_PAIR_OP(2, uint16_t, Func)
              DO_VECTOR_PAIR_OP(4, uint32_t, Func)
              DO_VECTOR_PAIR_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VADDV: {
            auto Op = IROp->C<IR::IROp_VAddV>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto current, auto a) { return current + a; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_REDUCE_1SRC_OP(1, int8_t, Func, 0)
              DO_VECTOR_REDUCE_1SRC_OP(2, int16_t, Func, 0)
              DO_VECTOR_REDUCE_1SRC_OP(4, int32_t, Func, 0)
              DO_VECTOR_REDUCE_1SRC_OP(8, int64_t, Func, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->Header.ElementSize);
            break;
          }
          case IR::OP_VURAVG: {
            auto Op = IROp->C<IR::IROp_VURAvg>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto b) { return (a + b + 1) >> 1; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, uint8_t,  Func)
              DO_VECTOR_OP(2, uint16_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VABS: {
            auto Op = IROp->C<IR::IROp_VAbs>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a) { return std::abs(a); };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_OP(1, int8_t, Func)
              DO_VECTOR_1SRC_OP(2, int16_t, Func)
              DO_VECTOR_1SRC_OP(4, int32_t, Func)
              DO_VECTOR_1SRC_OP(8, int64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VFMUL: {
            auto Op = IROp->C<IR::IROp_VFMul>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto b) { return a * b; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(4, float, Func)
              DO_VECTOR_OP(8, double, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VFDIV: {
            auto Op = IROp->C<IR::IROp_VFDiv>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto b) { return a / b; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(4, float, Func)
              DO_VECTOR_OP(8, double, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VFMIN: {
            auto Op = IROp->C<IR::IROp_VFMin>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto b) { return std::min(a, b); };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(4, float, Func)
              DO_VECTOR_OP(8, double, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VFMAX: {
            auto Op = IROp->C<IR::IROp_VFMax>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto b) { return std::max(a, b); };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(4, float, Func)
              DO_VECTOR_OP(8, double, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VFRECP: {
            auto Op = IROp->C<IR::IROp_VFRecp>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a) { return 1.0 / a; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_OP(4, float, Func)
              DO_VECTOR_1SRC_OP(8, double, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VFSQRT: {
            auto Op = IROp->C<IR::IROp_VFSqrt>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a) { return std::sqrt(a); };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_OP(4, float, Func)
              DO_VECTOR_1SRC_OP(8, double, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VFRSQRT: {
            auto Op = IROp->C<IR::IROp_VFRSqrt>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a) { return 1.0 / std::sqrt(a); };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_OP(4, float, Func)
              DO_VECTOR_1SRC_OP(8, double, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          #define DO_VECTOR_1SRC_2TYPE_OP(size, type, type2, func, min, max)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src_d = reinterpret_cast<type2*>(Src); \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = (type)func(Src_d[i], min, max);          \
            }                                                 \
            break;                                            \
            }
          #define DO_VECTOR_1SRC_2TYPE_OP_TOP(size, type, type2, func, min, max)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src_d = reinterpret_cast<type2*>(Src2); \
            memcpy(Dst_d, Src1, Elements * sizeof(type2));\
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i+Elements] = (type)func(Src_d[i], min, max);          \
            }                                                 \
            break;                                            \
            }

          #define DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(size, type, type2, func, min, max)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src_d = reinterpret_cast<type2*>(Src); \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = (type)func(Src_d[i+Elements], min, max);          \
            }                                                 \
            break;                                            \
            }
          #define DO_VECTOR_2SRC_2TYPE_OP(size, type, type2, func)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src1_d = reinterpret_cast<type2*>(Src1); \
            auto *Src2_d = reinterpret_cast<type2*>(Src2); \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = (type)func((type)Src1_d[i], (type)Src2_d[i]);          \
            }                                                 \
            break;                                            \
            }
          #define DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(size, type, type2, func)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src1_d = reinterpret_cast<type2*>(Src1); \
            auto *Src2_d = reinterpret_cast<type2*>(Src2); \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = (type)func((type)Src1_d[i+Elements], (type)Src2_d[i+Elements]);          \
            }                                                 \
            break;                                            \
            }

          case IR::OP_VUSHRNI: {
            auto Op = IROp->C<IR::IROp_VUShrNI>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t BitShift = Op->BitShift;
            uint8_t Tmp[16]{};

            uint8_t Elements = OpSize / (Op->Header.ElementSize << 1);

            auto Func = [BitShift](auto a, auto min, auto max) { return BitShift >= (sizeof(a) * 8) ? 0 : a >> BitShift; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP(1, uint8_t, uint16_t, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(2, uint16_t, uint32_t, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(4, uint32_t, uint64_t, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VUSHRNI2: {
            auto Op = IROp->C<IR::IROp_VUShrNI2>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t BitShift = Op->BitShift;
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / (Op->Header.ElementSize << 1);

            auto Func = [BitShift](auto a, auto min, auto max) { return BitShift >= (sizeof(a) * 8) ? 0 : a >> BitShift; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP_TOP(1, uint8_t, uint16_t, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP_TOP(2, uint16_t, uint32_t, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP_TOP(4, uint32_t, uint64_t, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VSQXTN: {
            auto Op = IROp->C<IR::IROp_VSQXTN>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Tmp[16]{};

            uint8_t Elements = OpSize / (Op->Header.ElementSize << 1);

            auto Func = [](auto a, auto min, auto max) { return std::max(std::min(a, (decltype(a))max), (decltype(a))min); };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP(1, int8_t, int16_t, Func, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max())
              DO_VECTOR_1SRC_2TYPE_OP(2, int16_t, int32_t, Func, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max())
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VSQXTN2: {
            auto Op = IROp->C<IR::IROp_VSQXTN2>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16]{};

            uint8_t Elements = OpSize / (Op->Header.ElementSize << 1);

            auto Func = [](auto a, auto min, auto max) { return std::max(std::min(a, (decltype(a))max), (decltype(a))min); };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP_TOP(1, int8_t, int16_t, Func, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max())
              DO_VECTOR_1SRC_2TYPE_OP_TOP(2, int16_t, int32_t, Func, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max())
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VSQXTUN: {
            auto Op = IROp->C<IR::IROp_VSQXTUN>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Tmp[16]{};

            uint8_t Elements = OpSize / (Op->Header.ElementSize << 1);

            auto Func = [](auto a, auto min, auto max) { return std::max(std::min(a, (decltype(a))max), (decltype(a))min); };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP(1, uint8_t, int16_t, Func, 0, (1 << 8) - 1)
              DO_VECTOR_1SRC_2TYPE_OP(2, uint16_t, int32_t, Func, 0, (1 << 16) - 1)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VSQXTUN2: {
            auto Op = IROp->C<IR::IROp_VSQXTUN2>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16]{};

            uint8_t Elements = OpSize / (Op->Header.ElementSize << 1);

            auto Func = [](auto a, auto min, auto max) { return std::max(std::min(a, (decltype(a))max), (decltype(a))min); };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP_TOP(1, uint8_t, int16_t, Func, 0, (1 << 8) - 1)
              DO_VECTOR_1SRC_2TYPE_OP_TOP(2, uint16_t, int32_t, Func, 0, (1 << 16) - 1)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VECTOR_UTOF: {
            auto Op = IROp->C<IR::IROp_Vector_UToF>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto min, auto max) { return a; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP(4, float, uint32_t, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(8, double, uint64_t, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VECTOR_STOF: {
            auto Op = IROp->C<IR::IROp_Vector_SToF>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto min, auto max) { return a; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP(4, float, int32_t, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(8, double, int64_t, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VECTOR_FTOZU: {
            auto Op = IROp->C<IR::IROp_Vector_FToZU>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto min, auto max) { return a; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP(4, uint32_t, float, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(8, uint64_t, double, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VECTOR_FTOZS: {
            auto Op = IROp->C<IR::IROp_Vector_FToZS>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto min, auto max) { return a; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP(4, int32_t, float, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(8, int64_t, double, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VECTOR_FTOU: {
            auto Op = IROp->C<IR::IROp_Vector_FToU>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto min, auto max) { return a; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP(4, uint32_t, float, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(8, uint64_t, double, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VECTOR_FTOS: {
            auto Op = IROp->C<IR::IROp_Vector_FToS>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto min, auto max) { return a; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP(4, int32_t, float, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(8, int64_t, double, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VUMUL: {
            auto Op = IROp->C<IR::IROp_VUMul>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto b) { return a * b; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, uint8_t,  Func)
              DO_VECTOR_OP(2, uint16_t, Func)
              DO_VECTOR_OP(4, uint32_t, Func)
              DO_VECTOR_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VSMUL: {
            auto Op = IROp->C<IR::IROp_VSMul>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto b) { return a * b; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, int8_t,  Func)
              DO_VECTOR_OP(2, int16_t, Func)
              DO_VECTOR_OP(4, int32_t, Func)
              DO_VECTOR_OP(8, int64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VUMULL: {
            auto Op = IROp->C<IR::IROp_VUMull>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);

            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto b) { return a * b; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_2SRC_2TYPE_OP(2, uint16_t, uint8_t, Func)
              DO_VECTOR_2SRC_2TYPE_OP(4, uint32_t, uint16_t, Func)
              DO_VECTOR_2SRC_2TYPE_OP(8, uint64_t, uint32_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VSMULL: {
            auto Op = IROp->C<IR::IROp_VSMull>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);

            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto b) { return a * b; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_2SRC_2TYPE_OP(2, int16_t, int8_t, Func)
              DO_VECTOR_2SRC_2TYPE_OP(4, int32_t, int16_t, Func)
              DO_VECTOR_2SRC_2TYPE_OP(8, int64_t, int32_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VUMULL2: {
            auto Op = IROp->C<IR::IROp_VUMull2>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);

            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto b) { return a * b; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(2, uint16_t, uint8_t, Func)
              DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(4, uint32_t, uint16_t, Func)
              DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(8, uint64_t, uint32_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VSMULL2: {
            auto Op = IROp->C<IR::IROp_VSMull2>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);

            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto b) { return a * b; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(2, int16_t, int8_t, Func)
              DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(4, int32_t, int16_t, Func)
              DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(8, int64_t, int32_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->Header.Size);
            break;
          }
          case IR::OP_VSXTL: {
            auto Op = IROp->C<IR::IROp_VSXTL>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Tmp[16]{};

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto min, auto max) { return a; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP(2, int16_t, int8_t, Func,  0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(4, int32_t, int16_t, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(8, int64_t, int32_t, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VSXTL2: {
            auto Op = IROp->C<IR::IROp_VSXTL2>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto min, auto max) { return a; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(2, int16_t, int8_t, Func,  0, 0)
              DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(4, int32_t, int16_t, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(8, int64_t, int32_t, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VUXTL: {
            auto Op = IROp->C<IR::IROp_VUXTL>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Tmp[16]{};

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto min, auto max) { return a; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP(2, uint16_t, uint8_t, Func,  0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(4, uint32_t, uint16_t, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(8, uint64_t, uint32_t, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VUXTL2: {
            auto Op = IROp->C<IR::IROp_VUXTL2>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);

            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;

            auto Func = [](auto a, auto min, auto max) { return a; };
            switch (Op->Header.ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(2, uint16_t, uint8_t, Func,  0, 0)
              DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(4, uint32_t, uint16_t, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(8, uint64_t, uint32_t, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VUMIN: {
            auto Op = IROp->C<IR::IROp_VUMin>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;
            auto Func = [](auto a, auto b) { return std::min(a, b); };

            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, uint8_t,  Func)
              DO_VECTOR_OP(2, uint16_t, Func)
              DO_VECTOR_OP(4, uint32_t, Func)
              DO_VECTOR_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VSMIN: {
            auto Op = IROp->C<IR::IROp_VSMin>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;
            auto Func = [](auto a, auto b) { return std::min(a, b); };

            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, int8_t,  Func)
              DO_VECTOR_OP(2, int16_t, Func)
              DO_VECTOR_OP(4, int32_t, Func)
              DO_VECTOR_OP(8, int64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VUMAX: {
            auto Op = IROp->C<IR::IROp_VUMax>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;
            auto Func = [](auto a, auto b) { return std::max(a, b); };

            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, uint8_t,  Func)
              DO_VECTOR_OP(2, uint16_t, Func)
              DO_VECTOR_OP(4, uint32_t, Func)
              DO_VECTOR_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VSMAX: {
            auto Op = IROp->C<IR::IROp_VSMax>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;
            auto Func = [](auto a, auto b) { return std::max(a, b); };

            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, int8_t,  Func)
              DO_VECTOR_OP(2, int16_t, Func)
              DO_VECTOR_OP(4, int32_t, Func)
              DO_VECTOR_OP(8, int64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VUSHL: {
            auto Op = IROp->C<IR::IROp_VUShl>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;
            auto Func = [](auto a, auto b) { return b >= (sizeof(a) * 8) ? 0 : a << b; };

            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, uint8_t,  Func)
              DO_VECTOR_OP(2, uint16_t, Func)
              DO_VECTOR_OP(4, uint32_t, Func)
              DO_VECTOR_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VSSHR: {
            auto Op = IROp->C<IR::IROp_VSShr>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;
            auto Func = [](auto a, auto b) { return b >= (sizeof(a) * 8) ? (a >> (sizeof(a) * 8 - 1)) : a >> b; };

            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, int8_t,  Func)
              DO_VECTOR_OP(2, int16_t, Func)
              DO_VECTOR_OP(4, int32_t, Func)
              DO_VECTOR_OP(8, int64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }

          case IR::OP_VUSHLS: {
            auto Op = IROp->C<IR::IROp_VUShlS>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;
            auto Func = [](auto a, auto b) { return b >= (sizeof(a) * 8) ? 0 : a << b; };

            switch (Op->Header.ElementSize) {
              DO_VECTOR_SCALAR_OP(1, uint8_t, Func)
              DO_VECTOR_SCALAR_OP(2, uint16_t, Func)
              DO_VECTOR_SCALAR_OP(4, uint32_t, Func)
              DO_VECTOR_SCALAR_OP(8, uint64_t, Func)
              DO_VECTOR_SCALAR_OP(16, __uint128_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VUSHRS: {
            auto Op = IROp->C<IR::IROp_VUShrS>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;
            auto Func = [](auto a, auto b) { return b >= (sizeof(a) * 8) ? 0 : a >> b; };

            switch (Op->Header.ElementSize) {
              DO_VECTOR_SCALAR_OP(1, uint8_t, Func)
              DO_VECTOR_SCALAR_OP(2, uint16_t, Func)
              DO_VECTOR_SCALAR_OP(4, uint32_t, Func)
              DO_VECTOR_SCALAR_OP(8, uint64_t, Func)
              DO_VECTOR_SCALAR_OP(16, __uint128_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VSSHRS: {
            auto Op = IROp->C<IR::IROp_VSShrS>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;
            auto Func = [](auto a, auto b) { return b >= (sizeof(a) * 8) ? (a >> (sizeof(a) * 8 - 1)) : a >> b; };

            switch (Op->Header.ElementSize) {
              DO_VECTOR_SCALAR_OP(1, int8_t, Func)
              DO_VECTOR_SCALAR_OP(2, int16_t, Func)
              DO_VECTOR_SCALAR_OP(4, int32_t, Func)
              DO_VECTOR_SCALAR_OP(8, int64_t, Func)
              DO_VECTOR_SCALAR_OP(16, __int128_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VUSHR: {
            auto Op = IROp->C<IR::IROp_VUShr>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;
            auto Func = [](auto a, auto b) { return b >= (sizeof(a) * 8) ? 0 : a >> b; };

            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, uint8_t,  Func)
              DO_VECTOR_OP(2, uint16_t, Func)
              DO_VECTOR_OP(4, uint32_t, Func)
              DO_VECTOR_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VZIP2:
          case IR::OP_VZIP: {
            auto Op = IROp->C<IR::IROp_VZip>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];
            uint8_t Elements = OpSize / Op->Header.ElementSize;
            uint8_t BaseOffset = IROp->Op == IR::OP_VZIP2 ? (Elements / 2) : 0;
            Elements >>= 1;

            switch (Op->Header.ElementSize) {
              case 1: {
                auto *Dst_d  = reinterpret_cast<uint8_t*>(Tmp);
                auto *Src1_d = reinterpret_cast<uint8_t*>(Src1);
                auto *Src2_d = reinterpret_cast<uint8_t*>(Src2);
                for (unsigned i = 0; i < Elements; ++i) {
                  Dst_d[i*2] = Src1_d[BaseOffset + i];
                  Dst_d[i*2+1] = Src2_d[BaseOffset + i];
                }
                break;
              }
              case 2: {
                auto *Dst_d  = reinterpret_cast<uint16_t*>(Tmp);
                auto *Src1_d = reinterpret_cast<uint16_t*>(Src1);
                auto *Src2_d = reinterpret_cast<uint16_t*>(Src2);
                for (unsigned i = 0; i < Elements; ++i) {
                  Dst_d[i*2] = Src1_d[BaseOffset + i];
                  Dst_d[i*2+1] = Src2_d[BaseOffset + i];
                }
                break;
              }
              case 4: {
                auto *Dst_d  = reinterpret_cast<uint32_t*>(Tmp);
                auto *Src1_d = reinterpret_cast<uint32_t*>(Src1);
                auto *Src2_d = reinterpret_cast<uint32_t*>(Src2);
                for (unsigned i = 0; i < Elements; ++i) {
                  Dst_d[i*2] = Src1_d[BaseOffset + i];
                  Dst_d[i*2+1] = Src2_d[BaseOffset + i];
                }
                break;
              }
              case 8: {
                auto *Dst_d  = reinterpret_cast<uint64_t*>(Tmp);
                auto *Src1_d = reinterpret_cast<uint64_t*>(Src1);
                auto *Src2_d = reinterpret_cast<uint64_t*>(Src2);
                for (unsigned i = 0; i < Elements; ++i) {
                  Dst_d[i*2] = Src1_d[BaseOffset + i];
                  Dst_d[i*2+1] = Src2_d[BaseOffset + i];
                }
                break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }

            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VINSELEMENT: {
            auto Op = IROp->C<IR::IROp_VInsElement>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            // Copy src1 in to dest
            memcpy(Tmp, Src1, OpSize);
            switch (Op->Header.ElementSize) {
              case 1: {
                auto *Dst_d  = reinterpret_cast<uint8_t*>(Tmp);
                auto *Src2_d = reinterpret_cast<uint8_t*>(Src2);
                Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];
                break;
              }
              case 2: {
                auto *Dst_d  = reinterpret_cast<uint16_t*>(Tmp);
                auto *Src2_d = reinterpret_cast<uint16_t*>(Src2);
                Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];
                break;
              }
              case 4: {
                auto *Dst_d  = reinterpret_cast<uint32_t*>(Tmp);
                auto *Src2_d = reinterpret_cast<uint32_t*>(Src2);
                Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];
                break;
              }
              case 8: {
                auto *Dst_d  = reinterpret_cast<uint64_t*>(Tmp);
                auto *Src2_d = reinterpret_cast<uint64_t*>(Src2);
                Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];
                break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            };
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VINSSCALARELEMENT: {
            auto Op = IROp->C<IR::IROp_VInsScalarElement>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            // Copy src1 in to dest
            memcpy(Tmp, Src1, OpSize);
            switch (Op->Header.ElementSize) {
              case 1: {
                auto *Dst_d  = reinterpret_cast<uint8_t*>(Tmp);
                auto Src2_d = *reinterpret_cast<uint8_t*>(Src2);
                Dst_d[Op->DestIdx] = Src2_d;
                break;
              }
              case 2: {
                auto *Dst_d  = reinterpret_cast<uint16_t*>(Tmp);
                auto Src2_d = *reinterpret_cast<uint16_t*>(Src2);
                Dst_d[Op->DestIdx] = Src2_d;
                break;
              }
              case 4: {
                auto *Dst_d  = reinterpret_cast<uint32_t*>(Tmp);
                auto Src2_d = *reinterpret_cast<uint32_t*>(Src2);
                Dst_d[Op->DestIdx] = Src2_d;
                break;
              }
              case 8: {
                auto *Dst_d  = reinterpret_cast<uint64_t*>(Tmp);
                auto Src2_d = *reinterpret_cast<uint64_t*>(Src2);
                Dst_d[Op->DestIdx] = Src2_d;
                break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            };
            memcpy(GDP, Tmp, OpSize);
            break;
          }

          case IR::OP_VBSL: {
            auto Op = IROp->C<IR::IROp_VBSL>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]);
            __uint128_t Src2 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[1]);
            __uint128_t Src3 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[2]);

            __uint128_t Tmp{};
            Tmp = Src2 & Src1;
            Tmp |= Src3 & ~Src1;

            memcpy(GDP, &Tmp, 16);
            break;
          }
          case IR::OP_VCMPEQ: {
            auto Op = IROp->C<IR::IROp_VCMPEQ>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;
            auto Func = [](auto a, auto b) { return a == b ? ~0ULL : 0; };

            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, uint8_t,   Func)
              DO_VECTOR_OP(2, uint16_t,  Func)
              DO_VECTOR_OP(4, uint32_t,  Func)
              DO_VECTOR_OP(8, uint64_t,  Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }

            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VCMPEQZ: {
            auto Op = IROp->C<IR::IROp_VCMPEQZ>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Src2[16]{};
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;
            auto Func = [](auto a, auto b) { return a == b ? ~0ULL : 0; };

            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, uint8_t,   Func)
              DO_VECTOR_OP(2, uint16_t,  Func)
              DO_VECTOR_OP(4, uint32_t,  Func)
              DO_VECTOR_OP(8, uint64_t,  Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }

            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VCMPGT: {
            auto Op = IROp->C<IR::IROp_VCMPGT>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;
            auto Func = [](auto a, auto b) { return a > b ? ~0ULL : 0; };

            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, int8_t,   Func)
              DO_VECTOR_OP(2, int16_t,  Func)
              DO_VECTOR_OP(4, int32_t,  Func)
              DO_VECTOR_OP(8, int64_t,  Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }

            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VCMPGTZ: {
            auto Op = IROp->C<IR::IROp_VCMPGTZ>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Src2[16]{};
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;
            auto Func = [](auto a, auto b) { return a > b ? ~0ULL : 0; };

            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, int8_t,   Func)
              DO_VECTOR_OP(2, int16_t,  Func)
              DO_VECTOR_OP(4, int32_t,  Func)
              DO_VECTOR_OP(8, int64_t,  Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }

            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VCMPLTZ: {
            auto Op = IROp->C<IR::IROp_VCMPLTZ>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Src2[16]{};
            uint8_t Tmp[16];

            uint8_t Elements = OpSize / Op->Header.ElementSize;
            auto Func = [](auto a, auto b) { return a < b ? ~0ULL : 0; };

            switch (Op->Header.ElementSize) {
              DO_VECTOR_OP(1, int8_t,   Func)
              DO_VECTOR_OP(2, int16_t,  Func)
              DO_VECTOR_OP(4, int32_t,  Func)
              DO_VECTOR_OP(8, int64_t,  Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }

            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_LUDIV: {
            auto Op = IROp->C<IR::IROp_LUDiv>();
            // Each source is OpSize in size
            // So you can have up to a 128bit divide from x86-64
            switch (OpSize) {
              case 2: {
                uint16_t SrcLow = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[0]);
                uint16_t SrcHigh = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[1]);
                uint16_t Divisor = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[2]);
                uint32_t Source = (static_cast<uint32_t>(SrcHigh) << 16) | SrcLow;
                uint32_t Res = Source / Divisor;

                // We only store the lower bits of the result
                GD = static_cast<uint16_t>(Res);
                break;
              }
              case 4: {
                uint32_t SrcLow = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[0]);
                uint32_t SrcHigh = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[1]);
                uint32_t Divisor = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[2]);
                uint64_t Source = (static_cast<uint64_t>(SrcHigh) << 32) | SrcLow;
                uint64_t Res = Source / Divisor;

                // We only store the lower bits of the result
                GD = static_cast<uint32_t>(Res);
                break;
              }
              case 8: {
                uint64_t SrcLow = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
                uint64_t SrcHigh = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
                uint64_t Divisor = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[2]);
                __uint128_t Source = (static_cast<__uint128_t>(SrcHigh) << 64) | SrcLow;
                __uint128_t Res = Source / Divisor;

                // We only store the lower bits of the result
                memcpy(GDP, &Res, OpSize);
                break;
              }
              default: LogMan::Msg::A("Unknown LUDIV Size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_LDIV: {
            auto Op = IROp->C<IR::IROp_LDiv>();
            // Each source is OpSize in size
            // So you can have up to a 128bit divide from x86-64
            switch (OpSize) {
              case 2: {
                uint16_t SrcLow = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[0]);
                uint16_t SrcHigh = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[1]);
                int16_t Divisor = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[2]);
                int32_t Source = (static_cast<uint32_t>(SrcHigh) << 16) | SrcLow;
                int32_t Res = Source / Divisor;

                // We only store the lower bits of the result
                GD = static_cast<int16_t>(Res);
                break;
              }
              case 4: {
                uint32_t SrcLow = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[0]);
                uint32_t SrcHigh = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[1]);
                int32_t Divisor = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[2]);
                int64_t Source = (static_cast<uint64_t>(SrcHigh) << 32) | SrcLow;
                int64_t Res = Source / Divisor;

                // We only store the lower bits of the result
                GD = static_cast<int32_t>(Res);
                break;
              }
              case 8: {
                uint64_t SrcLow = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
                uint64_t SrcHigh = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
                int64_t Divisor = *GetSrc<int64_t*>(SSAData, Op->Header.Args[2]);
                __int128_t Source = (static_cast<__int128_t>(SrcHigh) << 64) | SrcLow;
                __int128_t Res = Source / Divisor;

                // We only store the lower bits of the result
                memcpy(GDP, &Res, OpSize);
                break;
              }
              default: LogMan::Msg::A("Unknown LDIV Size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_LUREM: {
            auto Op = IROp->C<IR::IROp_LURem>();
            // Each source is OpSize in size
            // So you can have up to a 128bit Remainder from x86-64
            switch (OpSize) {
              case 2: {
                uint16_t SrcLow = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[0]);
                uint16_t SrcHigh = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[1]);
                uint16_t Divisor = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[2]);
                uint32_t Source = (static_cast<uint32_t>(SrcHigh) << 16) | SrcLow;
                uint32_t Res = Source % Divisor;

                // We only store the lower bits of the result
                GD = static_cast<uint16_t>(Res);
                break;
              }

              case 4: {
                uint32_t SrcLow = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[0]);
                uint32_t SrcHigh = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[1]);
                uint32_t Divisor = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[2]);
                uint64_t Source = (static_cast<uint64_t>(SrcHigh) << 32) | SrcLow;
                uint64_t Res = Source % Divisor;

                // We only store the lower bits of the result
                GD = static_cast<uint32_t>(Res);
                break;
              }
              case 8: {
                uint64_t SrcLow = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
                uint64_t SrcHigh = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
                uint64_t Divisor = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[2]);
                __uint128_t Source = (static_cast<__uint128_t>(SrcHigh) << 64) | SrcLow;
                __uint128_t Res = Source % Divisor;
                // We only store the lower bits of the result
                memcpy(GDP, &Res, OpSize);
                break;
              }
              default: LogMan::Msg::A("Unknown LUREM Size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_LREM: {
            auto Op = IROp->C<IR::IROp_LRem>();
            // Each source is OpSize in size
            // So you can have up to a 128bit Remainder from x86-64
            switch (OpSize) {
              case 2: {
                uint16_t SrcLow = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[0]);
                uint16_t SrcHigh = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[1]);
                int16_t Divisor = *GetSrc<uint16_t*>(SSAData, Op->Header.Args[2]);
                int32_t Source = (static_cast<uint32_t>(SrcHigh) << 16) | SrcLow;
                int32_t Res = Source % Divisor;

                // We only store the lower bits of the result
                GD = static_cast<int16_t>(Res);
                break;
              }
              case 4: {
                uint32_t SrcLow = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[0]);
                uint32_t SrcHigh = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[1]);
                int32_t Divisor = *GetSrc<uint32_t*>(SSAData, Op->Header.Args[2]);
                int64_t Source = (static_cast<uint64_t>(SrcHigh) << 32) | SrcLow;
                int64_t Res = Source % Divisor;

                // We only store the lower bits of the result
                GD = static_cast<int32_t>(Res);
                break;
              }
              case 8: {
                uint64_t SrcLow = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
                uint64_t SrcHigh = *GetSrc<uint64_t*>(SSAData, Op->Header.Args[1]);
                int64_t Divisor = *GetSrc<int64_t*>(SSAData, Op->Header.Args[2]);
                __int128_t Source = (static_cast<__int128_t>(SrcHigh) << 64) | SrcLow;
                __int128_t Res = Source % Divisor;
                // We only store the lower bits of the result
                memcpy(GDP, &Res, OpSize);
                break;
              }
              default: LogMan::Msg::A("Unknown LREM Size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_VEXTR: {
            auto Op = IROp->C<IR::IROp_VExtr>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]);
            __uint128_t Src2 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[1]);

            uint64_t Offset = Op->Index * Op->Header.ElementSize * 8;
            __uint128_t Dst{};
            if (Offset >= (OpSize * 8)) {
              Offset -= OpSize * 8;
              Dst = Src1 >> Offset;
            }
            else {
              Dst = (Src1 << (OpSize * 8 - Offset)) | (Src2 >> Offset);
            }

            memcpy(GDP, &Dst, OpSize);
            break;
          }
          case IR::OP_VINSGPR: {
            auto Op = IROp->C<IR::IROp_VInsGPR>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]);
            __uint128_t Src2 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[1]);

            uint64_t Offset = Op->Index * Op->Header.ElementSize * 8;
            __uint128_t Mask = (1ULL << (Op->Header.ElementSize * 8)) - 1;
            Mask <<= Offset;
            Mask = ~Mask;
            __uint128_t Dst = Src1 & Mask;
            Dst |= Src2 << Offset;

            memcpy(GDP, &Dst, OpSize);
            break;
          }
          case IR::OP_FLOAT_FROMGPR_S: {
            auto Op = IROp->C<IR::IROp_Float_FromGPR_S>();

            uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;
            switch (Conv) {
              case 0x0404: { // Float <- int32_t
                float Dst = (float)*GetSrc<int32_t*>(SSAData, Op->Header.Args[0]);
                memcpy(GDP, &Dst, Op->Header.ElementSize);
                break;
              }
              case 0x0408: { // Float <- int64_t
                float Dst = (float)*GetSrc<int64_t*>(SSAData, Op->Header.Args[0]);
                memcpy(GDP, &Dst, Op->Header.ElementSize);
                break;
              }
              case 0x0804: { // Double <- int32_t
                double Dst = (double)*GetSrc<int32_t*>(SSAData, Op->Header.Args[0]);
                memcpy(GDP, &Dst, Op->Header.ElementSize);
                break;
              }
              case 0x0808: { // Double <- int64_t
                double Dst = (double)*GetSrc<int64_t*>(SSAData, Op->Header.Args[0]);
                memcpy(GDP, &Dst, Op->Header.ElementSize);
                break;
              }
            }
            break;
          }
          case IR::OP_FLOAT_FROMGPR_U: {
            auto Op = IROp->C<IR::IROp_Float_FromGPR_U>();
            uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;
            switch (Conv) {
              case 0x0404: { // Float <- int32_t
                float Dst = (float)*GetSrc<uint32_t*>(SSAData, Op->Header.Args[0]);
                memcpy(GDP, &Dst, Op->Header.ElementSize);
                break;
              }
              case 0x0408: { // Float <- int64_t
                float Dst = (float)*GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
                memcpy(GDP, &Dst, Op->Header.ElementSize);
                break;
              }
              case 0x0804: { // Double <- int32_t
                double Dst = (double)*GetSrc<uint32_t*>(SSAData, Op->Header.Args[0]);
                memcpy(GDP, &Dst, Op->Header.ElementSize);
                break;
              }
              case 0x0808: { // Double <- int64_t
                double Dst = (double)*GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]);
                memcpy(GDP, &Dst, Op->Header.ElementSize);
                break;
              }
            }
            break;
          }
          case IR::OP_FLOAT_TOGPR_ZS: {
            auto Op = IROp->C<IR::IROp_Float_ToGPR_ZS>();
            if (Op->Header.ElementSize == 8) {
              int64_t Dst = (int64_t)*GetSrc<double*>(SSAData, Op->Header.Args[0]);
              memcpy(GDP, &Dst, Op->Header.ElementSize);
            }
            else {
              int32_t Dst = (int32_t)*GetSrc<float*>(SSAData, Op->Header.Args[0]);
              memcpy(GDP, &Dst, Op->Header.ElementSize);
            }
            break;
          }
          case IR::OP_FLOAT_TOGPR_ZU: {
            auto Op = IROp->C<IR::IROp_Float_ToGPR_ZU>();
            if (Op->Header.ElementSize == 8) {
              uint64_t Dst = (uint64_t)*GetSrc<double*>(SSAData, Op->Header.Args[0]);
              memcpy(GDP, &Dst, Op->Header.ElementSize);
            }
            else {
              uint32_t Dst = (uint32_t)*GetSrc<float*>(SSAData, Op->Header.Args[0]);
              memcpy(GDP, &Dst, Op->Header.ElementSize);
            }
            break;
          }
          case IR::OP_FLOAT_TOGPR_S: {
            auto Op = IROp->C<IR::IROp_Float_ToGPR_S>();
            if (Op->Header.ElementSize == 8) {
              int64_t Dst = (int64_t)*GetSrc<double*>(SSAData, Op->Header.Args[0]);
              memcpy(GDP, &Dst, Op->Header.ElementSize);
            }
            else {
              int32_t Dst = (int32_t)*GetSrc<float*>(SSAData, Op->Header.Args[0]);
              memcpy(GDP, &Dst, Op->Header.ElementSize);
            }
            break;
          }
          case IR::OP_FLOAT_TOGPR_U: {
            auto Op = IROp->C<IR::IROp_Float_ToGPR_U>();
            if (Op->Header.ElementSize == 8) {
              uint64_t Dst = (uint64_t)*GetSrc<double*>(SSAData, Op->Header.Args[0]);
              memcpy(GDP, &Dst, Op->Header.ElementSize);
            }
            else {
              uint32_t Dst = (uint32_t)*GetSrc<float*>(SSAData, Op->Header.Args[0]);
              memcpy(GDP, &Dst, Op->Header.ElementSize);
            }
            break;
          }
          case IR::OP_FLOAT_FTOF: {
            auto Op = IROp->C<IR::IROp_Float_FToF>();
            uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;
            switch (Conv) {
              case 0x0804: { // Double <- Float
                double Dst = (double)*GetSrc<float*>(SSAData, Op->Header.Args[0]);
                memcpy(GDP, &Dst, 8);
                break;
              }
              case 0x0408: { // Float <- Double
                float Dst = (float)*GetSrc<double*>(SSAData, Op->Header.Args[0]);
                memcpy(GDP, &Dst, 4);
                break;
              }
              default: LogMan::Msg::A("Unknown FCVT sizes: 0x%x", Conv);
            }
            break;
          }
          case IR::OP_VECTOR_FTOF: {
            auto Op = IROp->C<IR::IROp_Vector_FToF>();
            void *Src = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            uint8_t Tmp[16]{};

            uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;

            auto Func = [](auto a, auto min, auto max) { return a; };
            switch (Conv) {
              case 0x0804: { // Double <- float
                uint8_t Elements = OpSize / Op->SrcElementSize;
                switch (Op->SrcElementSize) {
                DO_VECTOR_1SRC_2TYPE_OP(4, double, float, Func, 0, 0)
                }
                break;
              }
              case 0x0408: { // Float <- Double
                uint8_t Elements = (OpSize << 1) / Op->SrcElementSize;
                switch (Op->SrcElementSize) {
                DO_VECTOR_1SRC_2TYPE_OP(8, float, double, Func, 0, 0)
                }
                break;

                break;
              }
              default: LogMan::Msg::A("Unknown Conversion Type : 0%04x", Conv); break;
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_FCMP: {
            auto Op = IROp->C<IR::IROp_FCmp>();
            uint32_t ResultFlags{};
            if (Op->ElementSize == 4) {
              float Src1 = *GetSrc<float*>(SSAData, Op->Header.Args[0]);
              float Src2 = *GetSrc<float*>(SSAData, Op->Header.Args[1]);
              bool Unordered = std::isnan(Src1) || std::isnan(Src2);
              if (Op->Flags & (1 << FCMP_FLAG_LT)) {
                if (Unordered || (Src1 < Src2)) {
                  ResultFlags |= (1 << FCMP_FLAG_LT);
                }
              }
              if (Op->Flags & (1 << FCMP_FLAG_UNORDERED)) {
                if (Unordered) {
                  ResultFlags |= (1 << FCMP_FLAG_UNORDERED);
                }
              }
              if (Op->Flags & (1 << FCMP_FLAG_EQ)) {
                if (Unordered || (Src1 == Src2)) {
                  ResultFlags |= (1 << FCMP_FLAG_EQ);
                }
              }
            }
            else {
              double Src1 = *GetSrc<double*>(SSAData, Op->Header.Args[0]);
              double Src2 = *GetSrc<double*>(SSAData, Op->Header.Args[1]);
              bool Unordered = std::isnan(Src1) || std::isnan(Src2);
              if (Op->Flags & (1 << FCMP_FLAG_LT)) {
                if (Unordered || (Src1 < Src2)) {
                  ResultFlags |= (1 << FCMP_FLAG_LT);
                }
              }
              if (Op->Flags & (1 << FCMP_FLAG_UNORDERED)) {
                if (Unordered) {
                  ResultFlags |= (1 << FCMP_FLAG_UNORDERED);
                }
              }
              if (Op->Flags & (1 << FCMP_FLAG_EQ)) {
                if (Unordered || (Src1 == Src2)) {
                  ResultFlags |= (1 << FCMP_FLAG_EQ);
                }
              }
            }

            GD = ResultFlags;
            break;
          }
          case IR::OP_VTBL1: {
            auto Op = IROp->C<IR::IROp_VTBL1>();
            uint8_t *Src1 = GetSrc<uint8_t*>(SSAData, Op->Header.Args[0]);
            uint8_t *Src2 = GetSrc<uint8_t*>(SSAData, Op->Header.Args[1]);

            uint8_t Tmp[16];

            for (size_t i = 0; i < OpSize; ++i) {
              uint8_t Index = Src2[i];
              Tmp[i] = Index >= OpSize ? 0 : Src1[Index];
            }
            memcpy(GDP, Tmp, OpSize);
            break;
          }

          case IR::OP_GETHOSTFLAG: {
            auto Op = IROp->C<IR::IROp_GetHostFlag>();
            GD = (*GetSrc<uint64_t*>(SSAData, Op->Header.Args[0]) >> Op->Flag) & 1;
            break;
          }
          #define DO_SCALAR_COMPARE_OP(size, type, type2, func)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type2*>(Tmp);  \
            auto *Src1_d = reinterpret_cast<type*>(Src1); \
            auto *Src2_d = reinterpret_cast<type*>(Src2); \
            Dst_d[0] = func(Src1_d[0], Src2_d[0]);          \
            break;                                            \
            }

          #define DO_VECTOR_COMPARE_OP(size, type, type2, func)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type2*>(Tmp);  \
            auto *Src1_d = reinterpret_cast<type*>(Src1); \
            auto *Src2_d = reinterpret_cast<type*>(Src2); \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = func(Src1_d[i], Src2_d[i]);          \
            }                                                 \
            break;                                            \
            }

          case IR::OP_VFCMPEQ: {
            auto Op = IROp->C<IR::IROp_VFCMPEQ>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);

            auto Func = [](auto a, auto b) { return a == b ? ~0ULL : 0; };

            uint8_t Tmp[16];
            uint8_t Elements = OpSize / Op->Header.ElementSize;

            if (Op->Header.ElementSize == OpSize) {
              switch (Op->Header.ElementSize) {
              DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
              DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
              default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
              }
            }
            else {
              switch (Op->Header.ElementSize) {
              DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
              DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
              default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
              }
            }

            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VFCMPNEQ: {
            auto Op = IROp->C<IR::IROp_VFCMPNEQ>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);

            auto Func = [](auto a, auto b) { return a != b ? ~0ULL : 0; };

            uint8_t Tmp[16];
            uint8_t Elements = OpSize / Op->Header.ElementSize;

            if (Op->Header.ElementSize == OpSize) {
              switch (Op->Header.ElementSize) {
              DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
              DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
              default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
              }
            }
            else {
              switch (Op->Header.ElementSize) {
              DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
              DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
              default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
              }
            }

            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VFCMPLT: {
            auto Op = IROp->C<IR::IROp_VFCMPLT>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);

            auto Func = [](auto a, auto b) { return a < b ? ~0ULL : 0; };

            uint8_t Tmp[16];
            uint8_t Elements = OpSize / Op->Header.ElementSize;

            if (Op->Header.ElementSize == OpSize) {
              switch (Op->Header.ElementSize) {
              DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
              DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
              default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
              }
            }
            else {
              switch (Op->Header.ElementSize) {
              DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
              DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
              default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
              }
            }

            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VFCMPLE: {
            auto Op = IROp->C<IR::IROp_VFCMPLE>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);

            auto Func = [](auto a, auto b) { return a <= b ? ~0ULL : 0; };

            uint8_t Tmp[16];
            uint8_t Elements = OpSize / Op->Header.ElementSize;

            if (Op->Header.ElementSize == OpSize) {
              switch (Op->Header.ElementSize) {
              DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
              DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
              default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
              }
            }
            else {
              switch (Op->Header.ElementSize) {
              DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
              DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
              default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
              }
            }

            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VFCMPUNO: {
            auto Op = IROp->C<IR::IROp_VFCMPUNO>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);

            auto Func = [](auto a, auto b) { return (std::isnan(a) || std::isnan(b)) ? ~0ULL : 0; };

            uint8_t Tmp[16];
            uint8_t Elements = OpSize / Op->Header.ElementSize;

            if (Op->Header.ElementSize == OpSize) {
              switch (Op->Header.ElementSize) {
              DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
              DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
              default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
              }
            }
            else {
              switch (Op->Header.ElementSize) {
              DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
              DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
              default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
              }
            }

            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VFCMPORD: {
            auto Op = IROp->C<IR::IROp_VFCMPORD>();
            void *Src1 = GetSrc<void*>(SSAData, Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(SSAData, Op->Header.Args[1]);

            auto Func = [](auto a, auto b) { return (!std::isnan(a) && !std::isnan(b)) ? ~0ULL : 0; };

            uint8_t Tmp[16];
            uint8_t Elements = OpSize / Op->Header.ElementSize;

            if (Op->Header.ElementSize == OpSize) {
              switch (Op->Header.ElementSize) {
              DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
              DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
              default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
              }
            }
            else {
              switch (Op->Header.ElementSize) {
              DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
              DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
              default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
              }
            }

            memcpy(GDP, Tmp, OpSize);
            break;
          }
          case IR::OP_VAESIMC: {
            auto Op = IROp->C<IR::IROp_VAESImc>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]);

            // Pseudo-code
            // Dst = InvMixColumns(STATE)
            __uint128_t Tmp{};
            Tmp = AES::InvMixColumns(reinterpret_cast<uint8_t*>(&Src1));
            memcpy(GDP, &Tmp, sizeof(Tmp));
            break;
          }
          case IR::OP_VAESENC: {
            auto Op = IROp->C<IR::IROp_VAESEnc>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]);
            __uint128_t Src2 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[1]);

            // Pseudo-code
            // STATE = Src1
            // RoundKey = Src2
            // STATE = ShiftRows(STATE)
            // STATE = SubBytes(STATE)
            // STATE = MixColumns(STATE)
            // Dst = STATE XOR RoundKey
            __uint128_t Tmp{};
            Tmp = AES::ShiftRows(reinterpret_cast<uint8_t*>(&Src1));
            Tmp = AES::SubBytes(reinterpret_cast<uint8_t*>(&Tmp), 16);
            Tmp = AES::MixColumns(reinterpret_cast<uint8_t*>(&Tmp));
            Tmp = Tmp ^ Src2;
            memcpy(GDP, &Tmp, sizeof(Tmp));
            break;
          }
          case IR::OP_VAESENCLAST: {
            auto Op = IROp->C<IR::IROp_VAESEncLast>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]);
            __uint128_t Src2 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[1]);

            // Pseudo-code
            // STATE = Src1
            // RoundKey = Src2
            // STATE = ShiftRows(STATE)
            // STATE = SubBytes(STATE)
            // Dst = STATE XOR RoundKey
            __uint128_t Tmp{};
            Tmp = AES::ShiftRows(reinterpret_cast<uint8_t*>(&Src1));
            Tmp = AES::SubBytes(reinterpret_cast<uint8_t*>(&Tmp), 16);
            Tmp = Tmp ^ Src2;
            memcpy(GDP, &Tmp, sizeof(Tmp));
            break;
          }
          case IR::OP_VAESDEC: {
            auto Op = IROp->C<IR::IROp_VAESDec>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]);
            __uint128_t Src2 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[1]);

            // Pseudo-code
            // STATE = Src1
            // RoundKey = Src2
            // STATE = InvShiftRows(STATE)
            // STATE = InvSubBytes(STATE)
            // STATE = InvMixColumns(STATE)
            // Dst = STATE XOR RoundKey
            __uint128_t Tmp{};
            Tmp = AES::InvShiftRows(reinterpret_cast<uint8_t*>(&Src1));
            Tmp = AES::InvSubBytes(reinterpret_cast<uint8_t*>(&Tmp));
            Tmp = AES::InvMixColumns(reinterpret_cast<uint8_t*>(&Tmp));
            Tmp = Tmp ^ Src2;
            memcpy(GDP, &Tmp, sizeof(Tmp));
            break;
          }
          case IR::OP_VAESDECLAST: {
            auto Op = IROp->C<IR::IROp_VAESDecLast>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[0]);
            __uint128_t Src2 = *GetSrc<__uint128_t*>(SSAData, Op->Header.Args[1]);

            // Pseudo-code
            // STATE = Src1
            // RoundKey = Src2
            // STATE = InvShiftRows(STATE)
            // STATE = InvSubBytes(STATE)
            // Dst = STATE XOR RoundKey
            __uint128_t Tmp{};
            Tmp = AES::InvShiftRows(reinterpret_cast<uint8_t*>(&Src1));
            Tmp = AES::InvSubBytes(reinterpret_cast<uint8_t*>(&Tmp));
            Tmp = Tmp ^ Src2;
            memcpy(GDP, &Tmp, sizeof(Tmp));
            break;
          }
          case IR::OP_VAESKEYGENASSIST: {
            auto Op = IROp->C<IR::IROp_VAESKeyGenAssist>();
            uint8_t *Src1 = GetSrc<uint8_t*>(SSAData, Op->Header.Args[0]);

            // Pseudo-code
            // X3 = Src1[127:96]
            // X2 = Src1[95:64]
            // X1 = Src1[63:32]
            // X0 = Src1[31:30]
            // RCON = (Zext)rcon
            // Dest[31:0] = SubWord(X1)
            // Dest[63:32] = RotWord(SubWord(X1)) XOR RCON
            // Dest[95:64] = SubWord(X3)
            // Dest[127:96] = RotWord(SubWord(X3)) XOR RCON
            __uint128_t Tmp{};
            uint32_t X1{};
            uint32_t X3{};
            memcpy(&X1, &Src1[4], 4);
            memcpy(&X3, &Src1[12], 4);
            uint32_t SubWord_X1 = AES::SubBytes(reinterpret_cast<uint8_t*>(&X1), 4);
            uint32_t SubWord_X3 = AES::SubBytes(reinterpret_cast<uint8_t*>(&X3), 4);

            auto Ror = [] (auto In, auto R) {
              auto RotateMask = sizeof(In) * 8 - 1;
              R &= RotateMask;
              return (In >> R) | (In << (sizeof(In) * 8 - R));
            };

            uint32_t Rot_X1 = Ror(SubWord_X1, 8);
            uint32_t Rot_X3 = Ror(SubWord_X3, 8);

            Tmp = Rot_X3 ^ Op->RCON;
            Tmp <<= 32;
            Tmp |= SubWord_X3;
            Tmp <<= 32;
            Tmp |= Rot_X1 ^ Op->RCON;
            Tmp <<= 32;
            Tmp |= SubWord_X1;
            memcpy(GDP, &Tmp, sizeof(Tmp));
            break;
          }
          case IR::OP_F80LOADFCW: {
            OpHandlers<IR::OP_F80LOADFCW>::handle(*GetSrc<uint16_t*>(SSAData, IROp->Args[0]));
            break;
          }
          case IR::OP_F80ADD: {
            auto Op = IROp->C<IR::IROp_F80Add>();
            X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);
            X80SoftFloat Src2 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[1]);
            X80SoftFloat Tmp;
            Tmp = X80SoftFloat::FADD(Src1, Src2);

            memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
            break;
          }
          case IR::OP_F80SUB: {
            auto Op = IROp->C<IR::IROp_F80Sub>();
            X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);
            X80SoftFloat Src2 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[1]);
            X80SoftFloat Tmp;
            Tmp = X80SoftFloat::FSUB(Src1, Src2);

            memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
            break;
          }
          case IR::OP_F80MUL: {
            auto Op = IROp->C<IR::IROp_F80Mul>();
            X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);
            X80SoftFloat Src2 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[1]);
            X80SoftFloat Tmp;
            Tmp = X80SoftFloat::FMUL(Src1, Src2);

            memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
            break;
          }
          case IR::OP_F80DIV: {
            auto Op = IROp->C<IR::IROp_F80Div>();
            X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);
            X80SoftFloat Src2 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[1]);
            X80SoftFloat Tmp;
            Tmp = X80SoftFloat::FDIV(Src1, Src2);

            memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
            break;
          }
          case IR::OP_F80FYL2X: {
            auto Op = IROp->C<IR::IROp_F80FYL2X>();
            X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);
            X80SoftFloat Src2 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[1]);
            X80SoftFloat Tmp;
            Tmp = X80SoftFloat::FYL2X(Src1, Src2);

            memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
            break;
          }
          case IR::OP_F80ATAN: {
            auto Op = IROp->C<IR::IROp_F80ATAN>();
            X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);
            X80SoftFloat Src2 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[1]);
            X80SoftFloat Tmp;
            Tmp = X80SoftFloat::FATAN(Src1, Src2);

            memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
            break;
          }
          case IR::OP_F80FPREM1: {
            auto Op = IROp->C<IR::IROp_F80FPREM1>();
            X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);
            X80SoftFloat Src2 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[1]);
            X80SoftFloat Tmp;
            Tmp = X80SoftFloat::FREM1(Src1, Src2);

            memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
            break;
          }
          case IR::OP_F80FPREM: {
            auto Op = IROp->C<IR::IROp_F80FPREM>();
            X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);
            X80SoftFloat Src2 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[1]);
            X80SoftFloat Tmp;
            Tmp = X80SoftFloat::FREM(Src1, Src2);

            memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
            break;
          }
          case IR::OP_F80SCALE: {
            auto Op = IROp->C<IR::IROp_F80SCALE>();
            X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);
            X80SoftFloat Src2 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[1]);
            X80SoftFloat Tmp;
            Tmp = X80SoftFloat::FSCALE(Src1, Src2);

            memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
            break;
          }
          case IR::OP_F80CVT: {
            auto Op = IROp->C<IR::IROp_F80CVT>();
            X80SoftFloat Src = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);

            switch (OpSize) {
              case 4: {
                float Tmp = Src;
                memcpy(GDP, &Tmp, OpSize);
                break;
              }
              case 8: {
                double Tmp = Src;
                memcpy(GDP, &Tmp, OpSize);
                break;
              }
            default: LogMan::Msg::D("Unhandled size: %d", OpSize);
            }
            break;
          }
          case IR::OP_F80CVTINT: {
            auto Op = IROp->C<IR::IROp_F80CVTInt>();
            X80SoftFloat Src = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);

            switch (OpSize) {
              case 2: {
                int16_t Tmp = (Op->Truncate? OpHandlers<IR::OP_F80CVTINT>::handle2t : OpHandlers<IR::OP_F80CVTINT>::handle2)(Src);
                memcpy(GDP, &Tmp, sizeof(Tmp));
                break;
              }
              case 4: {
                int32_t Tmp = (Op->Truncate? OpHandlers<IR::OP_F80CVTINT>::handle4t : OpHandlers<IR::OP_F80CVTINT>::handle4)(Src);
                memcpy(GDP, &Tmp, sizeof(Tmp));
                break;
              }
              case 8: {
                int64_t Tmp = (Op->Truncate? OpHandlers<IR::OP_F80CVTINT>::handle8t : OpHandlers<IR::OP_F80CVTINT>::handle8)(Src);
                memcpy(GDP, &Tmp, sizeof(Tmp));
                break;
              }
            default: LogMan::Msg::D("Unhandled size: %d", OpSize);
            }
            break;
          }
          case IR::OP_F80CVTTO: {
            auto Op = IROp->C<IR::IROp_F80CVTTo>();

            switch (Op->Size) {
              case 4: {
                float Src = *GetSrc<float *>(SSAData, Op->Header.Args[0]);
                X80SoftFloat Tmp = Src;
                memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
                break;
              }
              case 8: {
                double Src = *GetSrc<double *>(SSAData, Op->Header.Args[0]);
                X80SoftFloat Tmp = Src;
                memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
                break;
              }
            default: LogMan::Msg::D("Unhandled size: %d", OpSize);
            }
            break;
          }
          case IR::OP_F80CVTTOINT: {
            auto Op = IROp->C<IR::IROp_F80CVTToInt>();

            switch (Op->Size) {
              case 2: {
                int16_t Src = *GetSrc<int16_t*>(SSAData, Op->Header.Args[0]);
                X80SoftFloat Tmp = Src;
                memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
                break;
              }
              case 4: {
                int32_t Src = *GetSrc<int32_t*>(SSAData, Op->Header.Args[0]);
                X80SoftFloat Tmp = Src;
                memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
                break;
              }
            default: LogMan::Msg::D("Unhandled size: %d", OpSize);
            }
            break;
          }
          case IR::OP_F80ROUND: {
            auto Op = IROp->C<IR::IROp_F80Round>();
            X80SoftFloat Src = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);
            X80SoftFloat Tmp;
            Tmp = X80SoftFloat::FRNDINT(Src);

            memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
            break;
          }
          case IR::OP_F80F2XM1: {
            auto Op = IROp->C<IR::IROp_F80F2XM1>();
            X80SoftFloat Src = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);
            X80SoftFloat Tmp;
            Tmp = X80SoftFloat::F2XM1(Src);

            memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
            break;
          }
          case IR::OP_F80TAN: {
            auto Op = IROp->C<IR::IROp_F80TAN>();
            X80SoftFloat Src = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);
            X80SoftFloat Tmp;
            Tmp = X80SoftFloat::FTAN(Src);

            memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
            break;
          }
          case IR::OP_F80SQRT: {
            auto Op = IROp->C<IR::IROp_F80SQRT>();
            X80SoftFloat Src = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);
            X80SoftFloat Tmp;
            Tmp = X80SoftFloat::FSQRT(Src);

            memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
            break;
          }
          case IR::OP_F80SIN: {
            auto Op = IROp->C<IR::IROp_F80SIN>();
            X80SoftFloat Src = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);
            X80SoftFloat Tmp;
            Tmp = X80SoftFloat::FSIN(Src);
            memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
            break;
          }
          case IR::OP_F80COS: {
            auto Op = IROp->C<IR::IROp_F80COS>();
            X80SoftFloat Src = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);
            X80SoftFloat Tmp;
            Tmp = X80SoftFloat::FCOS(Src);
            memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
            break;
          }
          case IR::OP_F80XTRACT_EXP: {
            auto Op = IROp->C<IR::IROp_F80XTRACT_EXP>();
            X80SoftFloat Src = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);
            X80SoftFloat Tmp;
            Tmp = X80SoftFloat::FXTRACT_EXP(Src);
            memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
            break;
          }
          case IR::OP_F80XTRACT_SIG: {
            auto Op = IROp->C<IR::IROp_F80XTRACT_SIG>();
            X80SoftFloat Src = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);
            X80SoftFloat Tmp;
            Tmp = X80SoftFloat::FXTRACT_SIG(Src);
            memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
            break;
          }
          case IR::OP_F80CMP: {
            auto Op = IROp->C<IR::IROp_F80Cmp>();
            uint32_t ResultFlags{};
            X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);
            X80SoftFloat Src2 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[1]);
            bool eq, lt, nan;
            X80SoftFloat::FCMP(Src1, Src2, &eq, &lt, &nan);
            if (Op->Flags & (1 << FCMP_FLAG_LT) &&
                lt) {
              ResultFlags |= (1 << FCMP_FLAG_LT);
            }
            if (Op->Flags & (1 << FCMP_FLAG_UNORDERED) &&
                nan) {
              ResultFlags |= (1 << FCMP_FLAG_UNORDERED);
            }
            if (Op->Flags & (1 << FCMP_FLAG_EQ) &&
                eq) {
              ResultFlags |= (1 << FCMP_FLAG_EQ);
            }

            GD = ResultFlags;
            break;
          }
          case IR::OP_GETROUNDINGMODE: {
            uint32_t GuestRounding{};
#ifdef _M_ARM_64
            uint64_t Tmp{};
            __asm(R"(
              mrs %[Tmp], FPCR;
            )"
            : [Tmp] "=r" (Tmp));
            // Extract the rounding
            // On ARM the ordering is different than on x86
            GuestRounding |= ((Tmp >> 24) & 1) ? ROUND_MODE_FLUSH_TO_ZERO : 0;
            uint8_t RoundingMode = (Tmp >> 22) & 0b11;
            if (RoundingMode == 0)
              GuestRounding |= ROUND_MODE_NEAREST;
            else if (RoundingMode == 1)
              GuestRounding |= ROUND_MODE_POSITIVE_INFINITY;
            else if (RoundingMode == 2)
              GuestRounding |= ROUND_MODE_NEGATIVE_INFINITY;
            else if (RoundingMode == 3)
              GuestRounding |= ROUND_MODE_TOWARDS_ZERO;
#else
            GuestRounding = _mm_getcsr();

            // Extract the rounding
            GuestRounding = (GuestRounding >> 13) & 0b111;
#endif
            memcpy(GDP, &GuestRounding, sizeof(GuestRounding));
            break;
          }

          case IR::OP_SETROUNDINGMODE: {
            auto Op = IROp->C<IR::IROp_SetRoundingMode>();
            uint8_t GuestRounding = *GetSrc<uint8_t*>(SSAData, Op->Header.Args[0]);
#ifdef _M_ARM_64
            uint64_t HostRounding{};
            __asm volatile(R"(
              mrs %[Tmp], FPCR;
            )"
            : [Tmp] "=r" (HostRounding));
            // Mask out the rounding
            HostRounding &= ~(0b111 << 22);

            HostRounding |= (GuestRounding & ROUND_MODE_FLUSH_TO_ZERO) ? (1U << 24) : 0;

            uint8_t RoundingMode = GuestRounding & 0b11;
            if (RoundingMode == ROUND_MODE_NEAREST)
              HostRounding |= (0b00U << 22);
            else if (RoundingMode == ROUND_MODE_POSITIVE_INFINITY)
              HostRounding |= (0b01U << 22);
            else if (RoundingMode == ROUND_MODE_NEGATIVE_INFINITY)
              HostRounding |= (0b10U << 22);
            else if (RoundingMode == ROUND_MODE_TOWARDS_ZERO)
              HostRounding |= (0b11U << 22);

            __asm volatile(R"(
              msr FPCR, %[Tmp];
            )"
            :: [Tmp] "r" (HostRounding));
#else
            uint32_t HostRounding = _mm_getcsr();

            // Cut out the host rounding mode
            HostRounding &= ~(0b111 << 13);

            // Insert our new rounding mode
            HostRounding |= GuestRounding << 13;
            _mm_setcsr(HostRounding);
#endif
            break;
          }
          case IR::OP_F80BCDLOAD: {
            auto Op = IROp->C<IR::IROp_F80BCDLoad>();
            uint8_t *Src1 = GetSrc<uint8_t*>(SSAData, Op->Header.Args[0]);
            uint64_t BCD{};
            // We walk through each uint8_t and pull out the BCD encoding
            // Each 4bit split is a digit
            // Only 0-9 is supported, A-F results in undefined data
            // | 4 bit     | 4 bit    |
            // | 10s place | 1s place |
            // EG 0x48 = 48
            // EG 0x4847 = 4847
            // This gives us an 18digit value encoded in BCD
            // The last byte lets us know if it negative or not
            for (size_t i = 0; i < 9; ++i) {
              uint8_t Digit = Src1[8 - i];
              // First shift our last value over
              BCD *= 100;

              // Add the tens place digit
              BCD += (Digit >> 4) * 10;

              // Add the ones place digit
              BCD += Digit & 0xF;
            }

            // Set negative flag once converted to x87
            bool Negative = Src1[9] & 0x80;
            X80SoftFloat Tmp;

            Tmp = BCD;
            Tmp.Sign = Negative;

            memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
            break;
          }
          case IR::OP_F80BCDSTORE: {
            auto Op = IROp->C<IR::IROp_F80BCDStore>();
            X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(SSAData, Op->Header.Args[0]);
            bool Negative = Src1.Sign;

            // Clear the Sign bit
            Src1.Sign = 0;

            uint64_t Tmp = Src1;
            uint8_t BCD[10]{};

            for (size_t i = 0; i < 9; ++i) {
              if (Tmp == 0) {
                // Nothing left? Just leave
                break;
              }
              // Extract the lower 100 values
              uint8_t Digit = Tmp % 100;

              // Now divide it for the next iteration
              Tmp /= 100;

              uint8_t UpperNibble = Digit / 10;
              uint8_t LowerNibble = Digit % 10;

              // Now store the BCD
              BCD[i] = (UpperNibble << 4) | LowerNibble;
            }

            // Set negative flag once converted to x87
            BCD[9] = Negative ? 0x80 : 0;

            memcpy(GDP, BCD, 10);
            break;
          }
          default:
            LogMan::Msg::A("Unknown IR Op: %d(%s)", IROp->Op, FEXCore::IR::GetName(IROp->Op).data());
            break;
        }

        // CodeLast is inclusive. So we still need to dump the CodeLast op as well
        if (CodeBegin == CodeLast) {
          break;
        }
        ++CodeBegin;
      }
    };

    HandleBlock(BlockNode);

    if (BlockResults.Redo) {
      continue;
    }

    if (BlockResults.Quit || ++BlockIterator == BlockEnd) {
      break;
    }
  }
}

}
