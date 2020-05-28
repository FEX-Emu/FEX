%ifdef CONFIG
{
  "RegData": {
    "MM7":  ["0x8000000000000000", "0x4000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

lea rdx, [rel data]

fld tword [rdx + 8 * 0]
hlt

align 8
data:
  dt 2.0
  dq 0
