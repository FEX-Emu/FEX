%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4141414180000000",
    "RDX": "0x41414141FFFFFFFF",
    "RBX": "0xFFFFFFFF41424344",
    "RCX": "0xFFFFFFFF51525354",
    "R13": "0x5152535441424344",
    "R14": "0x1"
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
mov rax, 0x4141414180000000
mov rdx, 0x41414141FFFFFFFF

; Desired
mov rbx, 0xFFFFFFFF41424344
mov rcx, 0xFFFFFFFF51525354

cmpxchg8b [r15]

; Set r14 to 1 if if the memory location was expected
setz r14b

; Memory will now be set to the register data
; EDX:EAX will be the original data

; Check memory location to ensure it contains what we want
mov r13, [r15 + 8 * 0]
hlt
