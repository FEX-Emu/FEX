%ifdef CONFIG
{
  "RegData": {
    "MM7":  ["0x8000000000000000", "0x4001"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

fsqrt

hlt

align 8
data:
  dt 16.0
  dq 0
