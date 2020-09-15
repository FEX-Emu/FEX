%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x80000000",
    "RDX": "0xFFFFFFFF",
    "RBX": "0xFFFFFFFF41424344",
    "RCX": "0xFFFFFFFF51525354",
    "R13": "0xFFFFFFFF80000000",
    "R14": "0x0"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov r15, 0xe0000000

mov rax, 0xFFFFFFFF80000000
mov [r15 + 8 * 0], rax

mov r14, 0
; Expected
mov rax, 0x41414141FFFFFFFF
mov rdx, 0x41414141FFFFFFFF

; Desired
mov rbx, 0xFFFFFFFF41424344
mov rcx, 0xFFFFFFFF51525354

cmpxchg8b [r15]

; Set r14 to 1 if if the memory location was expected
setz r14b

; edx and eax will now contain the memory's data
; It will zext to the full 64bit of the register

; Check memory location to ensure it contains what we want
mov r13, [r15 + 8 * 0]
hlt
