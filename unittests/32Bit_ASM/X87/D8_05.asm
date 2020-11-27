%ifdef CONFIG
{
  "RegData": {
    "MM7":  ["0x8000000000000000", "0x3FFF"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [.data]

fld qword [edx + 8 * 0]
fsubr dword [edx + 8 * 1]
hlt

.data:
dq 0x3ff0000000000000
dq 0x40000000
