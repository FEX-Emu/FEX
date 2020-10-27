%ifdef CONFIG
{
  "RegData": {
    "MM6":  ["0x8000000000000000", "0x4000"],
    "MM7":  ["0x8000000000000000", "0x3FFF"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [data]
fld tword [edx + 8 * 0]

lea edx, [data2]
fld tword [edx + 8 * 0]

fsubr st1, st0

hlt

align 8
data:
  dt 1.0
  dq 0
data2:
  dt 2.0
  dq 0
