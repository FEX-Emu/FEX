%ifdef CONFIG
{
  "RegData": {
    "R15": "0x35"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

%macro cfmerge 0

; Get CF
sbb r14, r14
and r14, 1

; Merge in to results
shl r15, 1
or r15, r14

%endmacro

mov rdx, 0xe0000000

mov rax, 0xFFFFFFFF80000002
mov [rdx + 8 * 0], rax
mov [rdx + 8 * 1], rax
mov rax, 0x0
mov [rdx + 8 * 2], rax
mov rax, 0x01
mov [rdx + 8 * 3], eax
mov rax, 0x0
mov [rdx + 8 * 3 + 4], eax

xor r15, r15 ; Will contain our results
movzx r12, word [rdx]

; Test and set
bts r12w, 1
cfmerge

; Ensure it is set
bt r12w, 1
cfmerge

mov r13, 32
mov r12d, dword [rdx]

bts r12d, r13d
cfmerge

bt r12d, r13d
cfmerge

mov r12, qword [rdx]
bts r12, 64 * 3
cfmerge

bt r12, 64 * 3
cfmerge

hlt


