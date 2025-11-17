%ifdef CONFIG
{
  "RegData": {
    "RCX": "1",
    "RDI": "0xE0000007"
  },
  "Mode": "32BIT"
}
%endif

mov edx, 0xe0000000

mov eax, 0x45464748
mov [edx + 8 * 0], eax
mov eax, 0x41614344
mov [edx + 8 * 0 + 4], eax
mov eax, 0x55565758
mov [edx + 8 * 1], eax
mov eax, 0x51525354
mov [edx + 8 * 1 + 4], eax
mov eax, 0x0
mov [edx + 8 * 2], eax

lea edi, [edx + 8 * 0]

cld
mov eax, 0x61
mov ecx, 8
cmp eax, 0

repne scasb

hlt
