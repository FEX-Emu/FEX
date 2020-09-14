%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0021"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x8
mov [rdx + 8 * 0], rax

; Test that 8bit divide divides a 16bit dividend
mov ax, 0x0108
idiv byte [rdx + 8 * 0]

hlt
