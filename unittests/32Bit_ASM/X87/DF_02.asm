%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x400",
    "MM6": ["0x8000000000000000", "0x3FFF"],
    "MM7": ["0x8000000000000000", "0x4009"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [rel .data]

fld dword [edx + 8 * 0]

fist word [edx + 8 * 1]

fld1

mov eax, 0
mov ax, word [edx + 8 * 1]

hlt

align 4096
.data:
dq 0x44800000
dq -1
