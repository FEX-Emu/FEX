%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x414243444546FF72",
    "RBX": "0xFFFFFFFFFFFF0001"
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

mov al, -2
imul byte [rdx + 8 * 0 + 1]
mov word [rdx + 8 * 0], ax

; Ensure upper bits aren't cleared
mov rax, 0xFFFFFFFFFFFFFF01
mov rbx, 1
imul bl
mov rbx, rax

mov rax, [rdx + 8 * 0]

hlt
