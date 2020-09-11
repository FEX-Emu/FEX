%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x41424315",
    "RBX": "0x51525425",
    "RCX": "0x61626435"
  },
  "Mode": "32BIT"
}
%endif

mov esp, 0xe0000000

mov eax, 0x41424344
mov [esp + 4 * 0], eax
mov eax , 0x51525354
mov [esp + 4 * 1], eax
mov eax, 0x61626364
mov [esp + 4 * 2], eax

mov eax, 0xD1
add byte  [esp + 4 * 0], al
add word  [esp + 4 * 1], ax
add dword [esp + 4 * 2], eax

mov eax, [esp + 4 * 0]
mov ebx, [esp + 4 * 1]
mov ecx, [esp + 4 * 2]

hlt
