%ifdef CONFIG
{
  "RegData": {
    "MM6":  ["0xE666666666666668", "0xBFFE"],
    "MM7":  ["0xC000000000000000", "0x4000"]
  }
}
%endif

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

lea rdx, [rel data2]
fld tword [rdx + 8 * 0]

fprem1

hlt

align 4096
data:
  dt 3.0
  dq 0
data2:
  dt 5.1
  dq 0
