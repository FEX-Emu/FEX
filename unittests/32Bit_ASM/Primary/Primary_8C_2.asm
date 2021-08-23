%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFF0033",
    "RBX": "0x33",
    "RCX": "0x33"
  },
  "Mode": "32BIT"
}
%endif

mov eax, 0x33
mov gs, ax
mov fs, ax
mov es, ax

mov eax, 0xFFFFFFFF
mov ebx, 0xFFFFFFFF
mov ecx, 0xFFFFFFFF

; 16-bit insert
mov ax, gs
; 32-bit zext
mov ebx, fs
mov ecx, es

hlt
