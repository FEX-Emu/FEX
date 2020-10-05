%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x40",
    "RBX": "0x07",
    "RCX": "0x0F",
    "RDX": "0x17",
    "RSI": "0x1F",
    "RDI": "0x27",
    "RBP": "0x2F",
    "RSP": "0x37",
    "R8":  "0x3F",
    "R9":  "0x00",
    "R10": "0x08",
    "R11": "0x10",
    "R12": "0x18",
    "R13": "0x20",
    "R14": "0x28",
    "R15": "0x38"
  }
}
%endif

lea r15, [rel .data]

; We only care about results here
lzcnt rax, qword [r15 + 8 * 0]
lzcnt rbx, qword [r15 + 8 * 1]
lzcnt rcx, qword [r15 + 8 * 2]
lzcnt rdx, qword [r15 + 8 * 3]
lzcnt rsi, qword [r15 + 8 * 4]
lzcnt rdi, qword [r15 + 8 * 5]
lzcnt rbp, qword [r15 + 8 * 6]
lzcnt rsp, qword [r15 + 8 * 7]
lzcnt r8,  qword [r15 + 8 * 8]
lzcnt r9,  qword [r15 + 8 * 9]
lzcnt r10, qword [r15 + 8 * 10]
lzcnt r11, qword [r15 + 8 * 11]
lzcnt r12, qword [r15 + 8 * 12]
lzcnt r13, qword [r15 + 8 * 13]
lzcnt r14, qword [r15 + 8 * 14]
lzcnt r15, qword [r15 + 8 * 15]

hlt

.data:
dq 0x0000000000000000
dq 0x01FFFFFFFFFFFFFF
dq 0x0001FFFFFFFFFFFF
dq 0x000001FFFFFFFFFF
dq 0x00000001FFFFFFFF
dq 0x0000000001FFFFFF
dq 0x000000000001FFFF
dq 0x00000000000001FF
dq 0x0000000000000001
dq 0x8000000000000000
dq 0x0080000000000000
dq 0x0000800000000000
dq 0x0000008000000000
dq 0x0000000080000000
dq 0x0000000000800000
