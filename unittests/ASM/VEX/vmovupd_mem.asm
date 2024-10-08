%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "RAX": "0x6162636465666768",
    "RBX": "0x7172737475767778",
    "XMM0": ["0x4142434445464748", "0x5152535455565758", "0x6162636465666768", "0x7172737475767778"],
    "XMM1": ["0x4142434445464748", "0x5152535455565758", "0x6162636465666768", "0x7172737475767778"],
    "XMM2": ["0xCCCCCCCCCCCCCCCC", "0xDDDDDDDDDDDDDDDD", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax
mov rax, 0x6162636465666768
mov [rdx + 8 * 2], rax
mov rax, 0x7172737475767778
mov [rdx + 8 * 3], rax

mov rax, 0xCCCCCCCCCCCCCCCC
mov [rdx + 8 * 4], rax
mov rax, 0xDDDDDDDDDDDDDDDD
mov [rdx + 8 * 5], rax
mov rax, 0xEEEEEEEEEEEEEEEE
mov [rdx + 8 * 6], rax
mov rax, 0xFFFFFFFFFFFFFFFF
mov [rdx + 8 * 7], rax

; Test truncation
vmovupd ymm2, [rdx + 8 * 4]
vmovupd xmm2, [rdx + 8 * 4]

; Test memory overwrite
vmovupd ymm0, [rdx]
vmovupd [rdx + 8 * 4], ymm0
vmovupd ymm1, ymm0

mov rax, [rdx + 8 * 6]
mov rbx, [rdx + 8 * 7]

hlt
