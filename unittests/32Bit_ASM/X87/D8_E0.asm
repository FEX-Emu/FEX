%ifdef CONFIG
{
  "RegData": {
    "MM6":  ["0x8000000000000000", "0xBFFF"],
    "MM7":  ["0x8000000000000000", "0x4000"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [.data]

fld qword [edx + 8 * 0]
fld qword [edx + 8 * 1]
fsub st0, st1
hlt

.data:
dq 0x4000000000000000
dq 0x3ff0000000000000
