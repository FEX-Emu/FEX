%ifdef CONFIG
{
  "Match": "All",
  "RegData": {
    "XMM0": ["0xadbeefbad0dad100", "0x41414141414141de"],
    "XMM1": ["0x41deadbeefbad0da", "0x0041414141414141"]
  }
}
%endif

mov rdx, 0xe8000000
mov rax, 0xDEADBEEFBAD0DAD1
mov rcx, 0x4141414141414141

mov [rdx], rax
mov [rdx + 8], rcx

movups xmm0, [rdx]
pslldq xmm0, 1

movups xmm1, [rdx]
psrldq xmm1, 1

hlt
