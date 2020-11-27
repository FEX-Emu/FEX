%ifdef CONFIG
{
  "RegData": {
    "MM6":  ["0x8666666666666666", "0x4000"],
    "MM7":  ["0xC000000000000000", "0x4000"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [data]
fld tword [edx + 8 * 0]

lea edx, [data2]
fld tword [edx + 8 * 0]

fprem

hlt

align 8
data:
  dt 3.0
  dq 0
data2:
  dt 5.1
  dq 0
