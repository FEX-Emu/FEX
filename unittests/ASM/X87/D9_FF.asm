%ifdef CONFIG
{
  "RegData": {
    "MM7":  ["0xD51132BA9B902522", "0xBFFD"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

fcos

hlt

align 8
data:
  dt 2.0
  dq 0
