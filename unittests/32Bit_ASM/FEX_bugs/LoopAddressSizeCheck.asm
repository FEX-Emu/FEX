%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000000000001",
    "RBX": "0x0000000000010001"
  },
  "Mode": "32BIT"
}
%endif

; FEX-Emu had a bug where a16 loop instructions weren't treating the input RCX register as 16-bit.
; Effectively always treating it as 32-bit.
; Little test that operates at 16-bit and 32-bit sizes to ensure it is correctly handled.
mov eax, 0
mov ebx, 0
mov ecx, 0x0001_0001

.test:
inc eax
a16 loop .test

mov ecx, 0x0001_0001
.test2:
inc ebx
a32 loop .test2

hlt
