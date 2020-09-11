%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x81",
    "RBX": "0x8081",
    "RCX": "0x80808081"
  },
  "Mode": "32BIT"
}
%endif

mov eax, 0x01
add al, 0x80

mov ebx, 0x01
add bx, 0x8080

mov ecx, 0x01
add ecx, 0x80808080

hlt
