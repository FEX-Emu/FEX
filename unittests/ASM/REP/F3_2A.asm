%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x414243443f800000", "0x5152535455565758"],
    "XMM1":  ["0x4142434440000000", "0x5152535455565758"],
    "XMM2":  ["0x4142434440400000", "0x5152535455565758"],
    "XMM3":  ["0x4142434440800000", "0x5152535455565758"],
    "XMM4":  ["0x41424344C0800000", "0x5152535455565758"],
    "XMM5":  ["0x41424344C0800000", "0x5152535455565758"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

mov rax, 0x1
mov [rdx + 8 * 2], rax
mov rax, 0x2
mov [rdx + 8 * 3], rax
mov rax, 0x3
mov [rdx + 8 * 4], rax
mov rax, 0x4
mov [rdx + 8 * 5], rax

; Stick something in the top 32bits to ensure correctness
mov rax, 0x7fc00000FFFFFFFC
mov [rdx + 8 * 6], rax

movapd xmm0, [rdx + 8 * 0]
movapd xmm1, [rdx + 8 * 0]
movapd xmm2, [rdx + 8 * 0]
movapd xmm3, [rdx + 8 * 0]
movapd xmm4, [rdx + 8 * 0]
movapd xmm5, [rdx + 8 * 0]

mov rax, [rdx + 8 * 2]
mov rbx, [rdx + 8 * 3]

cvtsi2ss xmm0, rax
cvtsi2ss xmm1, ebx

cvtsi2ss xmm2, dword [rdx + 8 * 4]
cvtsi2ss xmm3, qword [rdx + 8 * 5]

mov rbx, [rdx + 8 * 6]

cvtsi2ss xmm4, ebx
cvtsi2ss xmm5, dword [rdx + 8 * 6]

hlt
