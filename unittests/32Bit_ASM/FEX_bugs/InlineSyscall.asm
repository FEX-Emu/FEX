%ifdef CONFIG
{
  "RegData": {
  },
  "Mode": "32BIT"
}
%endif

; FEX 32-bit inline syscalls hit an assert in uxtw
; Just use an inline syscall and throw it zero data to catch the assert
mov eax, 355 ; getrandom, is an inline syscall
mov ebx, 0
mov ecx, 0
mov edx, 0
int 0x80

hlt
