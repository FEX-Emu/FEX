%ifdef CONFIG
{
  "RegData": {
    "MM6": ["0x8000000000000000", "0x3FFF"],
    "MM7": ["0x8000000000000000", "0x4000"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [.data]

fld dword [edx + 8 * 0]
fld dword [edx + 8 * 1]

fxch

hlt

.data:
dq 0x3f800000
dq 0x40000000
