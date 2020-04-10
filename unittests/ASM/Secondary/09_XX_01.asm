%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x80000000",
    "RDX": "0xFFFFFFFF",
    "RBX": "0x41424344",
    "RCX": "0x51525354",
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
mov eax, 0x80000000
mov edx, 0xFFFFFFFF

; Desired
mov ebx, 0x41424344
mov ecx, 0x51525354

cmpxchg8b [r15]

; Set r14 to 1 if if the memory location was expected
setz r14b

; edx and eax will now contain the memory's data

; Check memory location to ensure it contains what we want
mov r13, [r15 + 8 * 0]
hlt
