%ifdef CONFIG
{
  "RegData": {
    "MM7":  ["0x8000000000000000", "0xFFFF"],
    "MM6":  ["0x0000000000000000", "0x0000"],    
    "MM5":  ["0x8000000000000000", "0xFFFF"],
    "MM4":  ["0x0000000000000000", "0x8000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

section .data
    nzer: dq -0.0

section .text
global _start
_start:
finit
fldz
fxtract ; MM7 is -inf, MM6 is 0.0

lea rdx, [rel nzer]
fld qword [rdx]
fxtract ; MM5 is -inf, MM4 is -0.0

hlt

