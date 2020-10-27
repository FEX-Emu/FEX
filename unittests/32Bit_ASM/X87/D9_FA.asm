%ifdef CONFIG
{
  "RegData": {
    "MM7":  ["0x8000000000000000", "0x4001"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [data]
fld tword [edx + 8 * 0]

fsqrt

hlt

align 8
data:
  dt 16.0
  dq 0
