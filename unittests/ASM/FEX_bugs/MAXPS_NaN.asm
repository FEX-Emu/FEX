%ifdef CONFIG
{
  "RegData": {
      "RAX": "0x40A00000",
      "RBX": "0x7FC00000",
      "RCX": "0x00000000",
      "RDX": "0x80000000",
      "RSI": "0x4014000000000000",
      "RDI": "0x7FF8000000000000",
      "R8":  "0x8000000000000000",
      "R9":  "0x0000000000000000",
      "R10": "0x40A00000",
      "R11": "0x7FC00000",
      "R12": "0x4014000000000000",
      "R13": "0x7FF8000000000000"
  },
  "HostFeatures": ["AVX"],
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

; MAXPS/MAXPD/MINPS/MINPD must return src2 when either operand is NaN
; or when the inputs are signed-zero ties. Exercise lane 0 with all
; four combinations.

bits 64

mov rbp, 0x100000000

mov dword [rbp +  0], 0x7FC00000
mov dword [rbp +  4], 0x7FC00000
mov dword [rbp +  8], 0x7FC00000
mov dword [rbp + 12], 0x7FC00000

mov dword [rbp + 16], 0x40A00000
mov dword [rbp + 20], 0x40A00000
mov dword [rbp + 24], 0x40A00000
mov dword [rbp + 28], 0x40A00000

mov dword [rbp + 32], 0x00000000
mov dword [rbp + 36], 0x00000000
mov dword [rbp + 40], 0x00000000
mov dword [rbp + 44], 0x00000000

mov dword [rbp + 48], 0x80000000
mov dword [rbp + 52], 0x80000000
mov dword [rbp + 56], 0x80000000
mov dword [rbp + 60], 0x80000000

mov rax, 0x7FF8000000000000
mov qword [rbp + 64], rax
mov qword [rbp + 72], rax

mov rax, 0x4014000000000000
mov qword [rbp + 80], rax
mov qword [rbp + 88], rax

xor eax, eax
mov qword [rbp +  96], rax
mov qword [rbp + 104], rax

mov rax, 0x8000000000000000
mov qword [rbp + 112], rax
mov qword [rbp + 120], rax

movups xmm0, [rbp +  0]
movups xmm1, [rbp + 16]
maxps  xmm0, xmm1
movd   eax, xmm0

movups xmm0, [rbp + 16]
movups xmm1, [rbp +  0]
maxps  xmm0, xmm1
movd   ebx, xmm0

movups xmm0, [rbp + 48]
movups xmm1, [rbp + 32]
maxps  xmm0, xmm1
movd   ecx, xmm0

movups xmm0, [rbp + 32]
movups xmm1, [rbp + 48]
maxps  xmm0, xmm1
movd   edx, xmm0

movups xmm0, [rbp + 64]
movups xmm1, [rbp + 80]
maxpd  xmm0, xmm1
movq   rsi, xmm0

movups xmm0, [rbp + 80]
movups xmm1, [rbp + 64]
maxpd  xmm0, xmm1
movq   rdi, xmm0

movups xmm0, [rbp + 96]
movups xmm1, [rbp + 112]
maxpd  xmm0, xmm1
movq   r8, xmm0

movups xmm0, [rbp + 112]
movups xmm1, [rbp +  96]
maxpd  xmm0, xmm1
movq   r9, xmm0

movups xmm0, [rbp +  0]
movups xmm1, [rbp + 16]
minps  xmm0, xmm1
movd   r10d, xmm0

movups xmm0, [rbp + 16]
movups xmm1, [rbp +  0]
minps  xmm0, xmm1
movd   r11d, xmm0

movups xmm0, [rbp + 64]
movups xmm1, [rbp + 80]
minpd  xmm0, xmm1
movq   r12, xmm0

movups xmm0, [rbp + 80]
movups xmm1, [rbp + 64]
minpd  xmm0, xmm1
movq   r13, xmm0

hlt
