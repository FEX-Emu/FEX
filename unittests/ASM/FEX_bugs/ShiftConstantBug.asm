%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x500000020"
  }
}
%endif

; FEX had a bug in its `TestNZ` opcode where it would try to load a constant in to the tst instruction
; If the constant didn't fit in a logical encoding it would generate invalid instructions and also crash.
; This snippet of code was found in libGLX.so.0.0.0 when trying to load steamwebhelper.
mov     eax, 0x28000001
shl     rax, 0x5

hlt
