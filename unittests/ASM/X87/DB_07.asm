%ifdef CONFIG
{
  "RegData": {
    "MM7":  ["0x8000000000000000", "0x4000"]
  }
}
%endif

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

lea rdx, [rel data2]
fstp tword [rdx + 8 * 0]
fld tword [rdx + 8 * 0]

hlt

align 4096
data:
  dt 2.0
  dq 0
data2:
  dt 0.0
  dq 0
