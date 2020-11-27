%ifdef CONFIG
{
  "RegData": {
    "MM6":  ["0x8000000000000000", "0x4006"],
    "MM7":  ["0xB000000000000000", "0x4001"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [data2]
fld tword [edx + 8 * 0]

lea edx, [data]
fld tword [edx + 8 * 0]

fscale

hlt

align 8
data:
  dt 4.0
  dq 0

data2:
  dt 5.5
  dq 0
