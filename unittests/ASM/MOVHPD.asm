%ifdef CONFIG
{
  "Match": "All",
  "RegData": {
    "RAX": "0xDEADBEEFBAD0DAD1",
    "RCX": "0xDEADBEEFBAD0DAD1",
    "XMM0": ["0", "0xDEADBEEFBAD0DAD1"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

; Data we want to store
mov rax, 0xDEADBEEFBAD0DAD1

; Starting address to store to
mov rdi, 0xe8000000

pxor xmm0, xmm0
pxor xmm1, xmm1

mov [rdi], rax

movhpd xmm0, [rdi]
movhpd [rdi + 8], xmm0

xor rcx, rcx
mov rcx, [rdi + 8]

hlt

