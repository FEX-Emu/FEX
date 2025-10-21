%ifdef CONFIG
{
  "RegData": {
    "MM7":  ["0x8000000000000000", "0x4000"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [rel data]
fld tword [edx + 8 * 0]

lea edx, [rel data2]
fstp tword [edx + 8 * 0]
fld tword [edx + 8 * 0]

hlt

align 4096
data:
  dt 2.0
  dq 0
data2:
  dt 0.0
  dq 0
