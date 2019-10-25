#pragma once
#include <FEXCore/Debug/X86Tables.h>

#include "LogManager.h"

namespace FEXCore::X86Tables {

#ifndef NDEBUG
extern uint64_t Total;
extern uint64_t NumInsts;
#endif

struct U8U8InfoStruct {
  uint8_t first, second;
  X86InstInfo Info;
};

struct U16U8InfoStruct {
  uint16_t first;
  uint8_t second;
  X86InstInfo Info;
};

static inline void GenerateTable(X86InstInfo *FinalTable, U8U8InfoStruct const *LocalTable, size_t TableSize) {
  for (size_t j = 0; j < TableSize; ++j) {
    U8U8InfoStruct const &Op = LocalTable[j];
    auto OpNum = Op.first;
    X86InstInfo const &Info = Op.Info;
    for (uint32_t i = 0; i < Op.second; ++i) {
      LogMan::Throw::A(FinalTable[OpNum + i].Type == TYPE_UNKNOWN, "Duplicate Entry %s->%s", FinalTable[OpNum + i].Name, Info.Name);
      FinalTable[OpNum + i] = Info;
#ifndef NDEBUG
      ++Total;
      if (Info.Type == TYPE_INST)
        NumInsts++;
#endif
    }
  }
};

static inline void GenerateTable(X86InstInfo *FinalTable, U16U8InfoStruct const *LocalTable, size_t TableSize) {
  for (size_t j = 0; j < TableSize; ++j) {
    U16U8InfoStruct const &Op = LocalTable[j];
    auto OpNum = Op.first;
    X86InstInfo const &Info = Op.Info;
    for (uint32_t i = 0; i < Op.second; ++i) {
      LogMan::Throw::A(FinalTable[OpNum + i].Type == TYPE_UNKNOWN, "Duplicate Entry %s->%s", FinalTable[OpNum + i].Name, Info.Name);
      FinalTable[OpNum + i] = Info;
#ifndef NDEBUG
      ++Total;
      if (Info.Type == TYPE_INST)
        NumInsts++;
#endif
    }
  }
};

static inline void GenerateTableWithCopy(X86InstInfo *FinalTable, U8U8InfoStruct const *LocalTable, size_t TableSize, X86InstInfo *OtherLocal) {
  for (size_t j = 0; j < TableSize; ++j) {
    U8U8InfoStruct const &Op = LocalTable[j];
    auto OpNum = Op.first;
    X86InstInfo const &Info = Op.Info;
    for (uint32_t i = 0; i < Op.second; ++i) {
      LogMan::Throw::A(FinalTable[OpNum + i].Type == TYPE_UNKNOWN, "Duplicate Entry %s->%s", FinalTable[OpNum + i].Name, Info.Name);
      if (Info.Type == TYPE_COPY_OTHER) {
        FinalTable[OpNum + i] = OtherLocal[OpNum + i];
      }
      else {
        FinalTable[OpNum + i] = Info;
#ifndef NDEBUG
        ++Total;
        if (Info.Type == TYPE_INST)
          NumInsts++;
#endif
      }
    }
  }
};

static inline void GenerateX87Table(X86InstInfo *FinalTable, U16U8InfoStruct const *LocalTable, size_t TableSize) {
  for (size_t j = 0; j < TableSize; ++j) {
    U16U8InfoStruct const &Op = LocalTable[j];
    auto OpNum = Op.first;
    X86InstInfo const &Info = Op.Info;
    for (uint32_t i = 0; i < Op.second; ++i) {
      LogMan::Throw::A(FinalTable[OpNum + i].Type == TYPE_UNKNOWN, "Duplicate Entry %s->%s", FinalTable[OpNum + i].Name, Info.Name);
      if ((OpNum & 0b11'000'000) == 0b11'000'000) {
        // If the mod field is 0b11 then it is a regular op
        FinalTable[OpNum + i] = Info;
      }
      else {
        // If the mod field is !0b11 then this instruction is duplicated through the whole mod [0b00, 0b10] range
        // and the modrm.rm space because that is used part of the instruction encoding
        LogMan::Throw::A((OpNum & 0b11'000'000) == 0, "Only support mod field of zero in this path");
        for (uint16_t mod = 0b00'000'000; mod < 0b11'000'000; mod += 0b01'000'000) {
          for (uint16_t rm = 0b000; rm < 0b1'000; ++rm) {
            FinalTable[(OpNum | mod | rm) + i] = Info;
          }
        }
      }
#ifndef NDEBUG
        ++Total;
        if (Info.Type == TYPE_INST)
          NumInsts++;
#endif
    }
  }
};

}

