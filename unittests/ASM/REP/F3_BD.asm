%ifdef CONFIG
{
  "RegData": {
    "RAX": "0",
    "RCX": "0x10",
    "RDX": "0x20",
    "RSI": "0x40",
    "RDI": "0",
    "RBP": "0",
    "RSP": "0",
    "R8":  "0x8",
    "R9":  "0x10",
    "R10": "0x20",
    "R15": "0x2A540"
  }
}
%endif

; Uses AX and BX and stores result in r15
; CF:ZF
%macro zfcfmerge 0
  lahf

  ; Shift CF to zero
  shr ax, 8

  ; Move to a temp
  mov bx, ax
  and rbx, 1

  shl r15, 1
  or r15, rbx

  shl r15, 1

  ; Move to a temp
  mov bx, ax

  ; Extract ZF
  shr bx, 6
  and rbx, 1

  ; Insert ZF
  or r15, rbx
%endmacro

mov rax, 0x80000001
cpuid

shr ecx, 5
and ecx, 1
cmp ecx, 1
je .continue

; We don't support the instruction. Leave
mov rax, 0xDEADBEEF41414141
hlt

.continue:

mov rax, 0
mov rbx, 0
mov r15, 0

; Test zeroes
mov rcx, 0
lzcnt cx, cx
zfcfmerge

mov rdx, 0
lzcnt edx, edx
zfcfmerge

mov rsi, 0
lzcnt rsi, rsi
zfcfmerge

; Test highest bit set to 1
mov rdi, 0x8000
lzcnt di, di
zfcfmerge

mov rbp, 0x80000000
lzcnt ebp, ebp
zfcfmerge

mov rsp, 0x8000000000000000
lzcnt rsp, rsp
zfcfmerge

; Test bit in the middle of the range
mov r8, 0x0080
lzcnt r8w, r8w
zfcfmerge

mov r9, 0x00008000
lzcnt r9d, r9d
zfcfmerge

mov r10, 0x00000080000000
lzcnt r10, r10
zfcfmerge

mov rax, 0

hlt
