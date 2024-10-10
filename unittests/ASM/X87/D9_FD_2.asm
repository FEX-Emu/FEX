%ifdef CONFIG
{
  "RegData": {
    "MM6":  ["0x8000000000000000", "0x3FFF"],
    "MM7":  ["0xD000000000000000", "0xC001"]
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
  dt 64.0
  dq 0

data2:
  dt -6.5
  dq 0
