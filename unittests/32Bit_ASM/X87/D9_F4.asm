%ifdef CONFIG
{
  "RegData": {
    "MM6":  ["0xF000000000000000", "0xBFFF"],
    "MM7":  ["0xC000000000000000", "0x4000"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [data]
fld tword [edx + 8 * 0]

fxtract

hlt

align 8
data:
  dt -15.0
  dq 0
