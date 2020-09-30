%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x414243441F968610",
    "RBX": "0x25D1437D318C1BE0",
    "RCX": "0xFFFFFFFFFFFF0004",
    "RDX": "0x0000000000000004",
    "RSI": "0xFC1B5FC85401D0C0",
    "RSP": "0x2B27F79B13618682"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov r15, 0xe0000000

mov rax, 0x4142434445464748
mov [r15 + 8 * 0], rax
mov rax, 0x5152535455565758
mov [r15 + 8 * 1], rax
mov rax, 0x6162636465666768
mov [r15 + 8 * 2], rax

mov ax, 0x7172
mul word [r15 + 8 * 0 + 0]
mov word [r15 + 8 * 0 + 0], ax
mov word [r15 + 8 * 0 + 2], dx

mov eax, 0x71727374
mul dword [r15 + 8 * 1 + 0]
mov dword [r15 + 8 * 1 + 0], eax
mov dword [r15 + 8 * 1 + 4], edx

mov rax, 0x7172737475767778
mul qword [r15 + 8 * 2 + 0]
mov rsi, rax
mov rsp, rdx

; Ensure zext handling is correct
; 16bit
mov rax, 0xFFFFFFFFFFFF0002
mov rbx, 0xFFFFFFFFFFFF0002
mul bx
mov rcx, rax

; 32bit
mov rax, 0xFFFFFFFF00000002
mov rbx, 0xFFFFFFFF00000002
mul ebx
mov rdx, rax

mov rax, [r15 + 8 * 0]
mov rbx, [r15 + 8 * 1]

hlt
