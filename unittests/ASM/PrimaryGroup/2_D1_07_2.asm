%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x414141414141FFFF",
    "RBX": "0x00000000FFFFFFFF",
    "RDX": "0xFFFFFFFFFFFFFFFF",
    "RSI": "0x42424242424242FF"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4141414141418000
mov rbx, 0x80000000
mov rdx, 0x8000000000000000
mov rsi, 0x4242424242424280

mov cl, 7
sar sil, cl

mov cl, 15
sar ax, cl

mov cl, 31
sar ebx, cl

mov cl, 63
sar rdx, cl

hlt
