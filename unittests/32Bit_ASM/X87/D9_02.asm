%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x3F800000",
    "MM7": ["0x8000000000000000", "0x3fff"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [rel .data]

fld dword [edx + 8 * 0]
fst dword [edx + 8 * 1]

mov eax, [edx + 8 * 1]

hlt

align 4096
.data:
dq 0x3f800000
dq 0
