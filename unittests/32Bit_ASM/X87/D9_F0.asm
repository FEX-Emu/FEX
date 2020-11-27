%ifdef CONFIG
{
  "RegData": {
    "MM7":  ["0x0000000000000000", "0x0000"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [data]
fld tword [edx + 8 * 0]
f2xm1

hlt

align 8
data:
  dt 0.0
  dq 0
