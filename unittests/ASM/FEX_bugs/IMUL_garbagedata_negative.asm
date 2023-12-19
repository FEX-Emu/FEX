%ifdef CONFIG
{
  "RegData": {
    "R15": "0x0000000000003fff"
  }
}
%endif

%macro cfmerge 0

; Get CF
lahf
shr rax, 8
and rax, 1

; Merge in to results
shl r15, 1
or r15, rax

%endmacro

; FEX had a bug where imul flag calculation was incorrect.
; CF and OF are set due to overflow.

mov r15, 0
mov rax, 0

; Multiply starting value
mov ebx, 0x6D

; imul 1-src
mov ebx, 0xaaaaaaab
mov ecx, 0x6D
imul cx, bx
cfmerge

mov ebx, 0xaaaaaaab
mov ecx, 0x6D
imul ecx, ebx
cfmerge

mov rbx, 0xaaaaaaaa_aaaaaaab
mov rcx, 0x6D
imul rcx, rbx
cfmerge

; imul 2-src 8-bit check
mov ebx, 0xaaaaaaab
imul cx, bx, 0x6D
cfmerge

mov ebx, 0xaaaaaaab
imul ecx, ebx, 0x6D
cfmerge

mov rbx, 0xaaaaaaaa_aaaaaaab
imul ecx, ebx, 0x6D
cfmerge

; imul 2-src 16-bit, 32-bit, 64-bit check
mov rbx, 0xaaaaaaaa_aaaaaaab
imul cx, bx, 0x600D
cfmerge

mov rbx, 0xaaaaaaaa_aaaaaaab
imul ecx, ebx, 0x600D0000
cfmerge


mov rbx, 0xaaaaaaaa_aaaaaaab
imul rcx, rbx, 0x600D0000
cfmerge

mov rbx, 0x0aaaaaaa_aaaaaaab
imul rcx, rbx, -0x600D0000
cfmerge


; IMUL implicit dest
mov rax, 0x0aaaaaaa_aaaaaaab
mov ecx, 0x6D
imul cl
cfmerge

mov rax, 0x0aaaaaaa_aaaaaaab
mov ecx, 0x600D
imul cx
cfmerge

mov rax, 0x0aaaaaaa_aaaaaaab
mov ecx, 0x600D0000
imul ecx
cfmerge

mov rax, 0x0aaaaaaa_aaaaaaab
mov rcx, 0x600D0000_00000000
imul rcx
cfmerge

hlt
