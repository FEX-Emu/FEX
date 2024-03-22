%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000000010001",
    "RBX": "0x0000000000000001"
  }
}
%endif

; FEX-Emu had a bug in the 32-bit implementation of LOOP where it didn't handle 16-bit RCX correctly.
; For test coverage on the 64-bit side, ensure that both 64-bit and 32-bit operation works correctly.
mov rax, 0
mov rbx, 0
mov rcx, 0x0001_0001

.test:
inc rax
a64 loop .test

mov rcx, 0x1_0000_0001
.test2:
inc rbx
a32 loop .test2

hlt
