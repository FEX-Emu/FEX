%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4748",
    "RBX": "0x4546",
    "RCX": "0x4344",
    "RDX": "0x4142",
    "RBP": "0x5758",
    "RSI": "0x5556",
    "RDI": "0x5354",
    "RSP": "0x5152"
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

movapd xmm0, [rdx + 8 * 0]

mov rax, -1
mov rbx, -1
mov rcx, -1
mov rdx, -1
mov rbp, -1
mov rsi, -1
mov rdi, -1
mov rsp, -1

pextrw rax, xmm0, 0
pextrw rbx, xmm0, 1
pextrw rcx, xmm0, 2
pextrw rdx, xmm0, 3
pextrw rbp, xmm0, 4
pextrw rsi, xmm0, 5
pextrw rdi, xmm0, 6
pextrw rsp, xmm0, 7

hlt
