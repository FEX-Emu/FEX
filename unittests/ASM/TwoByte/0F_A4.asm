%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434457584857",
    "RBX": "0x6162636467687576",
    "RCX": "0x8788919291929394",
    "RDX": "0xA7A8919293949596",
    "RSI": "0xB1B2B3B4B5B6B7B8"
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

mov rax, 0x6162636465666768
mov [rdx + 8 * 2], rax
mov rax, 0x7172737475767778
mov [rdx + 8 * 3], rax

mov rax, 0x8182838485868788
mov [rdx + 8 * 4], rax
mov rax, 0x9192939495969798
mov [rdx + 8 * 5], rax
mov rax, 0xA1A2A3A4A5A6A7A8
mov [rdx + 8 * 6], rax
mov rax, 0xB1B2B3B4B5B6B7B8
mov [rdx + 8 * 7], rax

mov rax, [rdx + 8 * 1]
shld word [rdx + 8 * 0 + 0], ax, 8
shld word [rdx + 8 * 0 + 2], ax, 16
shld word [rdx + 8 * 0 + 4], ax, 32

mov rax, [rdx + 8 * 3]
shld dword [rdx + 8 * 2 + 0], eax, 16
shld dword [rdx + 8 * 2 + 4], eax, 32

mov rax, [rdx + 8 * 5]
shld qword [rdx + 8 * 4 + 0], rax, 16
shld qword [rdx + 8 * 4 + 0], rax, 32
shld qword [rdx + 8 * 6 + 0], rax, 48
shld qword [rdx + 8 * 7 + 0], rax, 64

mov rax, qword [rdx + 8 * 0]
mov rbx, qword [rdx + 8 * 2]
mov rcx, qword [rdx + 8 * 4]
mov rsi, qword [rdx + 8 * 7]
mov rdx, qword [rdx + 8 * 6]

hlt
