%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0xffffffffffffffff", "0x00000000ffff0000"],
    "XMM1": ["0xffffffffffffffff", "0x12348000ffff0000"]
  },
  "HostFeatures": ["SSE4.1"]
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

mov eax, 0
mov [rdx + 8 * 2 + 0], eax
mov eax, 0x7FFFFFFF
mov [rdx + 8 * 2 + 4], eax
mov eax, 0x80000000
mov [rdx + 8 * 3 + 0], eax
mov eax, 0xFFFFFFFF
mov [rdx + 8 * 3 + 4], eax

; Values that actually fit in to 16bit unsigned
mov eax, 0
mov [rdx + 8 * 4 + 0], eax
mov eax, 0xFFFF
mov [rdx + 8 * 4 + 4], eax
mov eax, 0x8000
mov [rdx + 8 * 5 + 0], eax
mov eax, 0x1234
mov [rdx + 8 * 5 + 4], eax

; Setup source
movapd xmm0, [rdx + 8 * 0]
movapd xmm1, [rdx + 8 * 0]

; Pack it
packusdw xmm0, [rdx + 8 * 2]
packusdw xmm1, [rdx + 8 * 4]

hlt
