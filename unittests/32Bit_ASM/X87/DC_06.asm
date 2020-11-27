%ifdef CONFIG
{
  "RegData": {
    "MM7":  ["0x8000000000000000", "0x3FFE"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [data]
fld tword [edx + 8 * 0]

lea edx, [data2]
fdiv qword [edx + 8 * 0]

hlt

align 8
data:
  dt 1.0
  dq 0
data2:
  dq 2.0
