%ifdef CONFIG
{
  "RegData": {
    "RSP": "0x6",
    "RAX": "0x1",
    "RBX": "0x2",
    "RCX": "0x3",
    "RDX": "0x4",
    "R8": "0x5",
    "RBP": "0x0",
    "R9": "0x1",
    "R10": "0x2",
    "R11": "0x3",
    "R12": "0x4",
    "R13": "0x5"
  },
  "MemoryRegions": {
    "0x500000000": "0x100000000"
  }
}
%endif

; FEX had a bug that caused the truncation from the address-size flag to be applied after adding the segment base, even
; though the flag is only supposed to apply to the offset itself.
rdfsbase r14
mov rdx, 0x500000008
mov qword [rdx - 8], 0x0
mov qword [rdx], 0x1
mov qword [rdx + 8], 0x2
mov qword [rdx + 0x10], 0x3
mov qword [rdx + 0x18], 0x4
mov qword [rdx + 0x20], 0x5
wrfsbase rdx
mov rdx, 0x5FFFFFFF8
mov qword [rdx], 0x6

mov r8, 0x500000010
mov r9, 0x10
a32 mov rsp, qword [fs:-16]
a32 mov rax, qword [fs:0]
a32 mov rbx, qword [fs:8]
a32 mov rcx, qword [fs:r8d]
a32 mov rdx, qword [fs:r8d + 8]
a32 mov r8, qword [fs:r8d + r9d]

mov r15, 0x10
mov rbp, qword [fs:-8]
mov r9, qword [fs:0]
mov r10, qword [fs:8]
mov r11, qword [fs:r15]
mov r12, qword [fs:r15 + 8]
mov r13, qword [fs:r15 + r15]

wrfsbase r14
hlt
