#pragma once

// All CFA opcodes from DWARF v4 standard
#define CFI_OP(high, low) ((high << 6) | low)
#define DW_CFA_nop                CFI_OP(0x0, 0x00)
#define DW_CFA_set_loc            CFI_OP(0x0, 0x01) // address
#define DW_CFA_advance_loc1       CFI_OP(0x0, 0x02) // 1-byte delta
#define DW_CFA_advance_loc2       CFI_OP(0x0, 0x03) // 2-byte delta
#define DW_CFA_advance_loc4       CFI_OP(0x0, 0x04) // 4-byte delta
#define DW_CFA_offset_extended    CFI_OP(0x0, 0x05) // ULEB128 register, ULEB128 offset
#define DW_CFA_restore_extended   CFI_OP(0x0, 0x06) // ULEB128 register
#define DW_CFA_undefined          CFI_OP(0x0, 0x07) // ULEB128 register
#define DW_CFA_same_value         CFI_OP(0x0, 0x08) // ULEB128 register
#define DW_CFA_register           CFI_OP(0x0, 0x09) // ULEB128 register
#define DW_CFA_remember_state     CFI_OP(0x0, 0x0a)
#define DW_CFA_restore_state      CFI_OP(0x0, 0x0b)
#define DW_CFA_def_cfa            CFI_OP(0x0, 0x0c) // ULEB128 register
#define DW_CFA_def_cfa_register   CFI_OP(0x0, 0x0d) // ULEB128 register
#define DW_CFA_def_cfa_offset     CFI_OP(0x0, 0x0e) // ULEB128 offset
#define DW_CFA_def_cfa_expression CFI_OP(0x0, 0x0f) // BLOCK
#define DW_CFA_expression         CFI_OP(0x0, 0x10) // ULEB128 offset, BLOCK
#define DW_CFA_offset_extended_sf CFI_OP(0x0, 0x11) // ULEB128 register, SLEB128 offset
#define DW_CFA_def_cfa_sf         CFI_OP(0x0, 0x12) // ULEB128 register, SLEB128 offset
#define DW_CFA_def_cfa_offset_sf  CFI_OP(0x0, 0x13) // SLEB128 offset
#define DW_CFA_val_offset         CFI_OP(0x0, 0x14) // ULEB128, ULEB128
#define DW_CFA_val_offset_sf      CFI_OP(0x0, 0x15) // ULEB128, SLEB128
#define DW_CFA_val_expression     CFI_OP(0x0, 0x16) // ULEB128, BLOCK

#define DW_CFA_lo_user            CFI_OP(0x0, 0x1c)
#define DW_CFA_hi_user            CFI_OP(0x0, 0x3f)

#define DW_CFA_advance_loc(n)     CFI_OP(0x1, n)
#define DW_CFA_offset(reg)        CFI_OP(0x2, reg)
#define DW_CFA_restore(reg)       CFI_OP(0x3, reg)

// Some DWARF expression ops from Dwarf v4 standard
#define DW_OP_deref   0x06
#define DW_OP_const1u 0x08
#define DW_OP_const1s 0x09
#define DW_OP_const2u 0x0a
#define DW_OP_const2s 0x0b
#define DW_OP_const4u 0x0c
#define DW_OP_const4s 0x0d
#define DW_OP_const8u 0x0e
#define DW_OP_const8s 0x0d
#define DW_OP_constu  0x0e // uLEB128
#define DW_OP_consts  0x0e // sLEB128

#define DW_OP_plus 0x22

#define DW_OP_reg(n)  (0x50 + n)

#define DW_OP_nop 0x96
