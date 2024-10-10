%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x80000000",
    "RDX": "0xFFFFFFFF",
    "RBX": "0x0000000080000000",
    "RCX": "0x00000000ffffffff",
    "R13": "0xffffffff80000000",
    "R14": "0x0"
  }
}
%endif

mov r15, 0xe0000000

mov rax, 0xFFFFFFFF80000000
mov [r15 + 8 * 0], rax

mov r14, 0
; Expected
mov eax, 0x41424344
mov edx, 0x51525354

; Desired
mov ebx, 0x80000000
mov ecx, 0xFFFFFFFF

; Memory is already Desired and NOT expected
; Finds bug in CAS on AArch64

cmpxchg8b [r15]

; Set r14 to 1 if if the memory location was expected
setz r14b

; edx and eax will now contain the memory's data

; Check memory location to ensure it contains what we want
mov r13, [r15 + 8 * 0]
hlt
