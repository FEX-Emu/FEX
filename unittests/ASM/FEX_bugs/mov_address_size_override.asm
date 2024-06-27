%ifdef CONFIG
{
  "RegData": {
    "RDX": "0x5152535455565758",
    "R8": "0x5152535455565758"
  }
}
%endif
; FEX-Emu had a bug where address size override was overriding destination and source sizes on operations not affecting memory.
; This showed up as a bug in OpenSSL where GCC was padding move instructions with the address size prefix, knowing that it wouldn't do anything.
; FEX interpreted this address size prefix as making the destination 32-bit resulting in zero-extending the 64-bit source.
; Ensure this doesn't happen again.
mov rdx, 0x414243444546748
mov r8, 0x5152535455565758
jmp .test
.test:

; Add a couple address size prefixes
db 0x67, 0x67
mov rdx, r8
hlt
