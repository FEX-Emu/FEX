%ifdef CONFIG
{
  "RegData": {
    "MM6":  ["0x8000000000000000", "0x4006"],
    "MM7":  ["0xB000000000000000", "0x4001"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

lea rdx, [rel data2]
fld tword [rdx + 8 * 0]

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

fscale

hlt

align 8
data:
  dt 4.0
  dq 0

data2:
  dt 5.5
  dq 0
