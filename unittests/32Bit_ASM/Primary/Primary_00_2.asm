%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xB5",
    "RBX": "0x53D5",
    "RCX": "0x616263F5"
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

mov eax, 0x71
mov ebx, 0x81
mov ecx, 0x91

add al,  byte  [esp + 4 * 0]
add bx,  word  [esp + 4 * 1]
add ecx, dword [esp + 4 * 2]

hlt
