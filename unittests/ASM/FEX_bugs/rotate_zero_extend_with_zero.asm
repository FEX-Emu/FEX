%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000045464748",
    "RBX": "0x0000000055565758",
    "RSI": "0x0000000065666768",
    "RDX": "0x0000000075767778",
    "RDI": "0x0000000085868788",
    "RSP": "0x0000000095969798",
    "RBP": "0x0000000015161718",
    "R8":  "0x0000000025262728",
    "R9":  "0x3132333435363738",
    "R10": "0x0000000035363738"
  },
  "HostFeatures": ["BMI2"]
}
%endif

; FEX-Emu had a bug where it forgot to zero extend 32-bit rotate operations even if the rotate value masked to zero.
; Do both immediate encoded rotates and CL encoded rotates to ensure it gets zero extended correctly.
; Tests:
; - rotate left
; - rotate right
; - rotate with carry left
; - rotate with carry right
; - BMI2 rotate right without affecting flags

mov rax, 0x4142434445464748
mov rbx, 0x5152535455565758
mov rsi, 0x6162636465666768
mov rdx, 0x7172737475767778

mov rdi, 0x8182838485868788
mov rsp, 0x9192939495969798
mov rbp, 0x1112131415161718
mov r8,  0x2122232425262728
mov r9,  0x3132333435363738
mov r10, 0xA1A2A3A4A5A6A7A8

; Rotate count that when masked by 32-bit operating size it becomes zero!
mov rcx, 0x41424344454647E0

jmp .test
.test:

; Test that 32-bit rotates that mask to zero don't zero the upper bits
rol eax, cl
ror ebx, cl

; Test with imm encoded as well
rol esi, 0xE0
ror edx, 0xE0

; Test rotate with carries as well
rcl edi, cl
rcr esp, cl

rcl ebp, 0xE0
rcr r8d, 0xE0

; Test RORX as well
rorx r10d, r9d, 0xE0

hlt
