%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464748",
    "RDX": "0x5152535455565758",
    "RBX": "0x6162636465666768",
    "RCX": "0x7172737475767778",
    "R12": "0x6162636465666768",
    "R13": "0x7172737475767778",
    "R14": "0x1"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov r15, 0xe0000000

mov rax, 0x4142434445464748
mov [r15 + 8 * 0], rax
mov rax, 0x5152535455565758
mov [r15 + 8 * 1], rax

mov r14, 0
; Expected
mov rax, 0x4142434445464748
mov rdx, 0x5152535455565758

; Desired
mov rbx, 0x6162636465666768
mov rcx, 0x7172737475767778

cmpxchg16b [r15]

; Set r14 to 1 if if the memory location was expected
setz r14b

; rdx and rax will now contain the memory's data

; Check memory location to ensure it contains what we want
mov r12, [r15 + 8 * 0]
mov r13, [r15 + 8 * 1]

hlt
