#include "Interface/Core/X86Tables/X86Tables.h"

namespace FEXCore::X86Tables {
using namespace InstFlags;

void InitializeX87Tables() {
#define OPD(op, modrmop) (((op - 0xD8) << 8) | modrmop)
#define OPDReg(op, reg) (((op - 0xD8) << 8) | (reg << 3))
  const U16U8InfoStruct X87OpTable[] = {
    // 0xD8
    {OPDReg(0xD8, 0), 1, X86InstInfo{"FADD",  TYPE_X87, FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xD8, 1), 1, X86InstInfo{"FMUL",  TYPE_X87, FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xD8, 2), 1, X86InstInfo{"FCOM",  TYPE_X87, FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xD8, 3), 1, X86InstInfo{"FCOMP", TYPE_X87, FLAGS_MODRM | FLAGS_POP, 0, nullptr}},
    {OPDReg(0xD8, 4), 1, X86InstInfo{"FSUB",  TYPE_X87, FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xD8, 5), 1, X86InstInfo{"FSUBR", TYPE_X87, FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xD8, 6), 1, X86InstInfo{"FDIV",  TYPE_X87, FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xD8, 7), 1, X86InstInfo{"FDIVR", TYPE_X87, FLAGS_MODRM, 0, nullptr}},
      //  / 0
      {OPD(0xD8, 0xC0), 8, X86InstInfo{"FADD", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 1
      {OPD(0xD8, 0xC8), 8, X86InstInfo{"FMUL", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 2
      {OPD(0xD8, 0xD0), 8, X86InstInfo{"FCOM", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 3
      {OPD(0xD8, 0xD8), 8, X86InstInfo{"FCOMP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 4
      {OPD(0xD8, 0xE0), 8, X86InstInfo{"FSUB", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 5
      {OPD(0xD8, 0xE8), 8, X86InstInfo{"FSUBR", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 6
      {OPD(0xD8, 0xF0), 8, X86InstInfo{"FDIV", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 7
      {OPD(0xD8, 0xF8), 8, X86InstInfo{"FDIVR", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    // 0xD9
    {OPDReg(0xD9, 0), 1, X86InstInfo{"FLD",     TYPE_INST, FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xD9, 1), 1, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xD9, 2), 1, X86InstInfo{"FST",     TYPE_X87, FLAGS_MODRM | FLAGS_SF_MOD_DST, 0, nullptr}},
    {OPDReg(0xD9, 3), 1, X86InstInfo{"FSTP",    TYPE_X87, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_POP, 0, nullptr}},
    {OPDReg(0xD9, 4), 1, X86InstInfo{"FLDENV",  TYPE_X87, FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xD9, 5), 1, X86InstInfo{"FLDCW",   TYPE_X87, GenFlagsSameSize(SIZE_16BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xD9, 6), 1, X86InstInfo{"FNSTENV", TYPE_X87, FLAGS_MODRM | FLAGS_SF_MOD_DST, 0, nullptr}},
    {OPDReg(0xD9, 7), 1, X86InstInfo{"FNSTCW",  TYPE_INST, GenFlagsSameSize(SIZE_16BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST, 0, nullptr}},
      //  / 0
      {OPD(0xD9, 0xC0), 8, X86InstInfo{"FLD",   TYPE_INST, FLAGS_NONE, 0, nullptr}},
      //  / 1
      {OPD(0xD9, 0xC8), 8, X86InstInfo{"FXCH",  TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 2
      {OPD(0xD9, 0xD0), 1, X86InstInfo{"FNOP",  TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xD1), 7, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 3
      {OPD(0xD9, 0xD8), 8, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 4
      {OPD(0xD9, 0xE0), 1, X86InstInfo{"FCHS", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xE1), 1, X86InstInfo{"FABS", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xE2), 2, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xE4), 1, X86InstInfo{"FTST", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xE5), 1, X86InstInfo{"FXAM", TYPE_INST,  FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xE6), 2, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 5
      {OPD(0xD9, 0xE8), 1, X86InstInfo{"FLD1", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xE9), 1, X86InstInfo{"FLDL2T", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xEA), 1, X86InstInfo{"FLDL2E", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xEB), 1, X86InstInfo{"FLDPI", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xEC), 1, X86InstInfo{"FLDLG2", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xED), 1, X86InstInfo{"FLDLN2", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xEE), 1, X86InstInfo{"FLDZ", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xEF), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 6
      {OPD(0xD9, 0xF0), 1, X86InstInfo{"F2XM1", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xF1), 1, X86InstInfo{"FYL2X", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xF2), 1, X86InstInfo{"FPTAN", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xF3), 1, X86InstInfo{"FPATAN", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xF4), 1, X86InstInfo{"FXTRACT", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xF5), 1, X86InstInfo{"FPREM1", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xF6), 1, X86InstInfo{"FDECSTP", TYPE_X87, FLAGS_POP, 0, nullptr}},
      {OPD(0xD9, 0xF7), 1, X86InstInfo{"FINCSTP", TYPE_X87, FLAGS_POP, 0, nullptr}},
      //  / 7
      {OPD(0xD9, 0xF8), 1, X86InstInfo{"FPREM", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xF9), 1, X86InstInfo{"FYL2XP1", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xFA), 1, X86InstInfo{"FSQRT", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xFB), 1, X86InstInfo{"FSINCOS", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xFC), 1, X86InstInfo{"FRNDINT", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xFD), 1, X86InstInfo{"FSCALE", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xFE), 1, X86InstInfo{"FSIN", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xFF), 1, X86InstInfo{"FCOS", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    // 0xDA
    {OPDReg(0xDA, 0), 1, X86InstInfo{"FIADD", TYPE_X87,  GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDA, 1), 1, X86InstInfo{"FIMUL", TYPE_X87,  GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDA, 2), 1, X86InstInfo{"FICOM", TYPE_X87,  GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDA, 3), 1, X86InstInfo{"FICOMP", TYPE_X87, GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM | FLAGS_POP, 0, nullptr}},
    {OPDReg(0xDA, 4), 1, X86InstInfo{"FISUB", TYPE_X87,  GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDA, 5), 1, X86InstInfo{"FISUBR", TYPE_X87, GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDA, 6), 1, X86InstInfo{"FIDIV", TYPE_X87,  GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDA, 7), 1, X86InstInfo{"FIDIVR", TYPE_X87, GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM, 0, nullptr}},
      //  / 0
      {OPD(0xDA, 0xC0), 8, X86InstInfo{"FCMOVB", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 1
      {OPD(0xDA, 0xC8), 8, X86InstInfo{"FCMOVE", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 2
      {OPD(0xDA, 0xD0), 8, X86InstInfo{"FCMOVBE", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 3
      {OPD(0xDA, 0xD8), 8, X86InstInfo{"FCMOVU", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 4
      {OPD(0xDA, 0xE0), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 5
      {OPD(0xDA, 0xE8), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      {OPD(0xDA, 0xE9), 1, X86InstInfo{"FUCOMPP", TYPE_X87, FLAGS_POP, 0, nullptr}},
      {OPD(0xDA, 0xEA), 6, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 6
      {OPD(0xDA, 0xF0), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 7
      {OPD(0xDA, 0xF8), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    // 0xDB
    {OPDReg(0xDB, 0), 1, X86InstInfo{"FILD",   TYPE_X87, GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDB, 1), 1, X86InstInfo{"FISTTP", TYPE_X87, GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_POP, 0, nullptr}},
    {OPDReg(0xDB, 2), 1, X86InstInfo{"FIST",   TYPE_X87, GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST, 0, nullptr}},
    {OPDReg(0xDB, 3), 1, X86InstInfo{"FISTP",  TYPE_X87, GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_POP, 0, nullptr}},
    {OPDReg(0xDB, 4), 1, X86InstInfo{"",       TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDB, 5), 1, X86InstInfo{"FLD",    TYPE_X87,    FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDB, 6), 1, X86InstInfo{"",       TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDB, 7), 1, X86InstInfo{"FSTP",   TYPE_X87,   FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_POP, 0, nullptr}},
      //  / 0
      {OPD(0xDB, 0xC0), 8, X86InstInfo{"FCMOVNB", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 1
      {OPD(0xDB, 0xC8), 8, X86InstInfo{"FCMOVNE", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 2
      {OPD(0xDB, 0xD0), 8, X86InstInfo{"FCMOVNBE", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 3
      {OPD(0xDB, 0xD8), 8, X86InstInfo{"FCMOVNU", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 4
      {OPD(0xDB, 0xE0), 2, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      {OPD(0xDB, 0xE2), 1, X86InstInfo{"FNCLEX", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xDB, 0xE3), 1, X86InstInfo{"FNINIT", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xDB, 0xE4), 4, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 5
      {OPD(0xDB, 0xE8), 8, X86InstInfo{"FUCOMI", TYPE_INST, FLAGS_NONE, 0, nullptr}},
      //  / 6
      {OPD(0xDB, 0xF0), 8, X86InstInfo{"FCOMI", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 7
      {OPD(0xDB, 0xF8), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    // 0xDC
    {OPDReg(0xDC, 0), 1, X86InstInfo{"FADD", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDC, 1), 1, X86InstInfo{"FMUL", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDC, 2), 1, X86InstInfo{"FCOM", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDC, 3), 1, X86InstInfo{"FCOMP", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_POP, 0, nullptr}},
    {OPDReg(0xDC, 4), 1, X86InstInfo{"FSUB", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDC, 5), 1, X86InstInfo{"FSUBR", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDC, 6), 1, X86InstInfo{"FDIV", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDC, 7), 1, X86InstInfo{"FDIVR", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM, 0, nullptr}},
      //  / 0
      {OPD(0xDC, 0xC0), 8, X86InstInfo{"FADD", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 1
      {OPD(0xDC, 0xC8), 8, X86InstInfo{"FMUL", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 2
      {OPD(0xDC, 0xD0), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 3
      {OPD(0xDC, 0xD8), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 4
      {OPD(0xDC, 0xE0), 8, X86InstInfo{"FSUBR", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 5
      {OPD(0xDC, 0xE8), 8, X86InstInfo{"FSUB", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 6
      {OPD(0xDC, 0xF0), 8, X86InstInfo{"FDIVR", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 7
      {OPD(0xDC, 0xF8), 8, X86InstInfo{"FDIV", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    // 0xDD
    {OPDReg(0xDD, 0), 1, X86InstInfo{"FLD", TYPE_X87, FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDD, 1), 1, X86InstInfo{"FISTTP", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_POP, 0, nullptr}},
    {OPDReg(0xDD, 2), 1, X86InstInfo{"FST", TYPE_X87, FLAGS_MODRM | FLAGS_SF_MOD_DST, 0, nullptr}},
    {OPDReg(0xDD, 3), 1, X86InstInfo{"FSTP", TYPE_X87, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_POP, 0, nullptr}},
    {OPDReg(0xDD, 4), 1, X86InstInfo{"FRSTOR", TYPE_X87, FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDD, 5), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDD, 6), 1, X86InstInfo{"FNSAVE", TYPE_X87, FLAGS_MODRM | FLAGS_SF_MOD_DST, 0, nullptr}},
    {OPDReg(0xDD, 7), 1, X86InstInfo{"FNSTSW", TYPE_X87, GenFlagsSameSize(SIZE_16BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST, 0, nullptr}},
      //  / 0
      {OPD(0xDD, 0xC0), 8, X86InstInfo{"FFREE", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 1
      {OPD(0xDD, 0xC8), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 2
      {OPD(0xDD, 0xD0), 8, X86InstInfo{"FST", TYPE_INST, FLAGS_SF_MOD_DST, 0, nullptr}},
      //  / 3
      {OPD(0xDD, 0xD8), 8, X86InstInfo{"FSTP", TYPE_X87, FLAGS_SF_MOD_DST | FLAGS_POP, 0, nullptr}},
      //  / 4
      {OPD(0xDD, 0xE0), 8, X86InstInfo{"FUCOM", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 5
      {OPD(0xDD, 0xE8), 8, X86InstInfo{"FUCOMP", TYPE_X87, FLAGS_POP, 0, nullptr}},
      //  / 6
      {OPD(0xDD, 0xF0), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 7
      {OPD(0xDD, 0xF8), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    // 0xDE
    {OPDReg(0xDE, 0), 1, X86InstInfo{"FIADD", TYPE_X87,  GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDE, 1), 1, X86InstInfo{"FIMUL", TYPE_X87,  GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDE, 2), 1, X86InstInfo{"FICOM", TYPE_X87,  GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDE, 3), 1, X86InstInfo{"FICOMP", TYPE_X87, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM | FLAGS_POP, 0, nullptr}},
    {OPDReg(0xDE, 4), 1, X86InstInfo{"FISUB", TYPE_X87,  GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDE, 5), 1, X86InstInfo{"FISUBR", TYPE_X87, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDE, 6), 1, X86InstInfo{"FIDIV", TYPE_X87,  GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDE, 7), 1, X86InstInfo{"FIDIVR", TYPE_X87, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM, 0, nullptr}},
      //  / 0
      {OPD(0xDE, 0xC0), 8, X86InstInfo{"FADDP", TYPE_X87, FLAGS_POP, 0, nullptr}},
      //  / 1
      {OPD(0xDE, 0xC8), 8, X86InstInfo{"FMULP", TYPE_X87, FLAGS_POP, 0, nullptr}},
      //  / 2
      {OPD(0xDE, 0xD0), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 3
      {OPD(0xDE, 0xD8), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      {OPD(0xDE, 0xD9), 1, X86InstInfo{"FCOMPP", TYPE_X87, FLAGS_POP, 0, nullptr}},
      {OPD(0xDE, 0xDA), 6, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 4
      {OPD(0xDE, 0xE0), 8, X86InstInfo{"FSUBRP", TYPE_X87, FLAGS_POP, 0, nullptr}},
      //  / 5
      {OPD(0xDE, 0xE8), 8, X86InstInfo{"FSUBP", TYPE_X87, FLAGS_POP, 0, nullptr}},
      //  / 6
      {OPD(0xDE, 0xF0), 8, X86InstInfo{"FDIVRP", TYPE_X87, FLAGS_POP, 0, nullptr}},
      //  / 7
      {OPD(0xDE, 0xF8), 8, X86InstInfo{"FDIVP", TYPE_X87, FLAGS_POP, 0, nullptr}},
    // 0xDF
    {OPDReg(0xDF, 0), 1, X86InstInfo{"FILD", TYPE_X87, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDF, 1), 1, X86InstInfo{"FISTTP", TYPE_X87, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_POP, 0, nullptr}},
    {OPDReg(0xDF, 2), 1, X86InstInfo{"FIST",   TYPE_X87, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST, 0, nullptr}},
    {OPDReg(0xDF, 3), 1, X86InstInfo{"FISTP",  TYPE_X87, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_POP, 0, nullptr}},
    {OPDReg(0xDF, 4), 1, X86InstInfo{"FBLD", TYPE_X87, FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDF, 5), 1, X86InstInfo{"FILD", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM, 0, nullptr}},
    {OPDReg(0xDF, 6), 1, X86InstInfo{"FBSTP", TYPE_X87, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_POP, 0, nullptr}},
    {OPDReg(0xDF, 7), 1, X86InstInfo{"FISTP", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_POP, 0, nullptr}},
      //  / 0
      //  This instruction is a bit special. This is an undocumented(Almost) x87 instruction.
      //  https://en.wikipedia.org/wiki/X86_instruction_listings#Undocumented_x87_instructions
      //  https://www.pagetable.com/?p=16
      //  AMD Athlon Processor x86 Code Optimization Guide - `Use FFREEP Macro to Pop One Register from the FPU Stack`
      //  ISA architecture manuals don't talk about this instruction at all
      //  At some point the Nvidia OpenGL binary driver uses this instruction.
      //  GCC may also end up emitting this instruction in some rare edge case!
      //  Almost all x86 CPUs implement this, and it is expected to be around
      {OPD(0xDF, 0xC0), 8, X86InstInfo{"FFREEP",  TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 1
      {OPD(0xDF, 0xC8), 8, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 2
      {OPD(0xDF, 0xD0), 8, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 3
      {OPD(0xDF, 0xD8), 8, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 4
      {OPD(0xDF, 0xE0), 1, X86InstInfo{"FNSTSW",  TYPE_INST, GenFlagsSameSize(SIZE_16BIT) | FLAGS_SF_DST_RAX, 0, nullptr}},
      {OPD(0xDF, 0xE1), 7, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 5
      {OPD(0xDF, 0xE8), 8, X86InstInfo{"FUCOMIP", TYPE_INST,    FLAGS_POP, 0, nullptr}},
      //  / 6
      {OPD(0xDF, 0xF0), 8, X86InstInfo{"FCOMIP",  TYPE_X87,   FLAGS_POP, 0, nullptr}},
      //  / 7
      {OPD(0xDF, 0xF8), 8, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
  };
#undef OPD
#undef OPDReg

  GenerateX87Table(X87Ops, X87OpTable, sizeof(X87OpTable) / sizeof(X87OpTable[0]));
}
}
