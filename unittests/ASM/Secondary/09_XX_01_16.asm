%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464748",
    "RDX": "0x5152535455565758",
    "RBX": "0x6162636465666768",
    "R15": "0x7172737475767778",
    "R12": "0x4142434445464748",
    "R13": "0x5152535455565758",
    "R14": "0x0"
  }
}
%endif

mov rcx, 0xe0000000

mov rax, 0x4142434445464748
mov [rcx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rcx + 8 * 1], rax

mov r14, 0
; Expected
mov rax, 0x41424344FFFFFFFF
mov rdx, 0x5152535455565758

; Desired
mov rbx, 0x6162636465666768
mov r15, 0x7172737475767778

; Prefix F2h, ensures it still operates at 16b
db 0xF2
cmpxchg16b [rcx]

; Set r14 to 1 if if the memory location was expected
setz r14b

; rdx and rax will now contain the memory's data

; Check memory location to ensure it contains what we want
mov r12, [rcx + 8 * 0]
mov r13, [rcx + 8 * 1]

hlt
