%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000080000000",
    "RDX": "0x00000000ffffffff",
    "RBX": "0x4141414180000000",
    "RCX": "0x41414141ffffffff",
    "R13": "0xffffffff80000000",
    "R14": "0x0"
  }
}
%endif

; Spans 64byte boundary and unaligned
mov r15, 0xe000003F

mov rax, 0xFFFFFFFF80000000
mov [r15 + 8 * 0], rax

mov r14, 0
; Expected
mov rax, 0xFFFFFFFF41424344
mov rdx, 0xFFFFFFFF51525354

; Desired
mov rbx, 0x4141414180000000
mov rcx, 0x41414141FFFFFFFF

; Memory is already Desired and NOT expected
; Finds bug in CAS on AArch64

cmpxchg8b [r15]

; Set r14 to 1 if if the memory location was expected
setz r14b

; Memory will now be set to the register data
; EDX:EAX will be the original data

; Check memory location to ensure it contains what we want
mov r13, [r15 + 8 * 0]
hlt
