%ifdef CONFIG
{
  "RegData": {
    "RBX": "0",
    "RCX": "8",
    "RDX": "0",
    "RSP": "0"
  }
}
%endif

; FEX-Emu had a bug where it thought repeat worked on increment and decrement instructions.
; While the prefix can be encoded on the instructions, it is ignored by the hardware implementation.
; This checks to ensure that inc/dec ignore the repeat prefix, and that rcx isn't ever changed from it.

mov rsp, 0
mov rcx, 8
lea rax, [rel .test]

rep inc rsp
rep inc byte [rax]

rep dec rsp
rep dec byte [rax]

mov rbx, [rel .test]
mov rdx, [rel .test + 8]

hlt

align 4096
.test:
db 0, 0, 0, 0, 0, 0, 0, 0
db 0, 0, 0, 0, 0, 0, 0, 0
