%ifdef CONFIG
{
  "RegData": {
    "MM6": ["0x8000000000000000", "0x4001"],
    "MM7": ["0x8000000000000000", "0x3FFE"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

lea rdx, [rel data2]
fld tword [rdx + 8 * 0]

; fdivp 2.0, 4.0
; == st1 = 2.0 / 4.0
fdivp st1, st0

lea rdx, [rel data3]
fld tword [rdx + 8 * 0]

hlt

align 8
data:
  dt 2.0
  dq 0
data2:
  dt 4.0
  dq 0
data3:
  dt 4.0
  dq 0
