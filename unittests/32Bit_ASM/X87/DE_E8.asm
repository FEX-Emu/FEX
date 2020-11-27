%ifdef CONFIG
{
  "RegData": {
    "MM6": ["0x8000000000000000", "0x4001"],
    "MM7": ["0x8000000000000000", "0xC000"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [data]
fld tword [edx + 8 * 0]

lea edx, [data2]
fld tword [edx + 8 * 0]
fsubp st1, st0

lea edx, [data3]
fld tword [edx + 8 * 0]

hlt

align 8
data:
  dt 2.0
  dq 0
data2:
  dt 4.0
  dq 0
data3:
  dt 4.0
  dq 0
