%ifdef CONFIG
{
  "RegData": {
    "MM6":  ["0x8A51407DA8345C92", "0x3FFE"],
    "MM7":  ["0xD76AA47848677021", "0x3FFE"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

fsincos

hlt

align 8
data:
  dt 1.0
  dq 0
