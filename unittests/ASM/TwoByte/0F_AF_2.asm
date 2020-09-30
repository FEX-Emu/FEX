%ifdef CONFIG
{
  "RegData": {
    "R15": "0x0000000000c00030"
  }
}
%endif

; Uses CX and BX and stores result in r15
; OF:CF
%macro ofcfmerge 0
  lahf

  ; Load OF
  mov rbx, 0
  seto bl

  shl r15, 1
  or r15, rbx
  shl r15, 1

  ; Insert CF
  shr ax, 8
  and rax, 1
  or r15, rax
%endmacro

mov r8, 0xe0000000
mov r15, 0

; Positive * Positive
mov rax, 128
mov rbx, 32
imul ax, bx
ofcfmerge

mov rax, 128
mov rbx, 32
imul eax, ebx
ofcfmerge

mov rax, 128
mov rbx, 32
imul rax, rbx
ofcfmerge

; Negative * Positive
mov rax, -128
mov rbx, 32
imul ax, bx
ofcfmerge

mov rax, -128
mov rbx, 32
imul eax, ebx
ofcfmerge

mov rax, -128
mov rbx, 32
imul rax, rbx
ofcfmerge

; Positive * Negative
mov rax, 128
mov rbx, -32
imul ax, bx
ofcfmerge

mov rax, 128
mov rbx, -32
imul eax, ebx
ofcfmerge

mov rax, 128
mov rbx, -32
imul rax, rbx
ofcfmerge

; Negative * Negative
mov rax, -128
mov rbx, -32
imul ax, bx
ofcfmerge

mov rax, -128
mov rbx, -32
imul eax, ebx
ofcfmerge

mov rax, -128
mov rbx, -32
imul rax, rbx
ofcfmerge

; Positive * Positive Overflow
mov rax, 128
mov rbx, 256
imul ax, bx
ofcfmerge

mov rax, 128
mov rbx, 256
imul eax, ebx
ofcfmerge

mov rax, 128
mov rbx, 256
imul rax, rbx
ofcfmerge

; Negative * Positive Overflow
mov rax, -128
mov rbx, 256
imul ax, bx
ofcfmerge

mov rax, -128
mov rbx, 256
imul eax, ebx
ofcfmerge

mov rax, -128
mov rbx, 256
imul rax, rbx
ofcfmerge

; Positive * Negative Overflow
mov rax, 128
mov rbx, -256
imul ax, bx
ofcfmerge


; XXX: Claiming this is an overflow
mov rax, 128
mov rbx, -256
imul eax, ebx
ofcfmerge

mov rax, 128
mov rbx, -256
imul rax, rbx
ofcfmerge

; Negative * Negative Overflow
mov rax, -128
mov rbx, -256
imul ax, bx
ofcfmerge

mov rax, -128
mov rbx, -256
imul eax, ebx
ofcfmerge

mov rax, -128
mov rbx, -256
imul rax, rbx
ofcfmerge

hlt
