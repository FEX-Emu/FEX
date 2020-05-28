%ifdef CONFIG
{
  "RegData": {
    "MM6":  ["0xF000000000000000", "0xBFFF"],
    "MM7":  ["0xC000000000000000", "0x4000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

fxtract

hlt

align 8
data:
  dt -15.0
  dq 0
