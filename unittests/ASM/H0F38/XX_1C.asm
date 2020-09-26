%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x0",
    "MM1": "0x0101010101010101",
    "MM2": "0x0101010101010101",
    "MM3": "0x0100010001010100"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x0000000000000000
mov [rdx + 8 * 0], rax
mov rax, 0xFFFFFFFFFFFFFFFF
mov [rdx + 8 * 1], rax
mov rax, 0x0101010101010101
mov [rdx + 8 * 2], rax
mov rax, 0xFF000100FF01FF00
mov [rdx + 8 * 3], rax

; Test with full zero
pabsb mm0, [rdx + 8 * 0]

; Test with full negative
pabsb mm1, [rdx + 8 * 1]

; Test with full positive
pabsb mm2, [rdx + 8 * 2]

; Test a mix
pabsb mm3, [rdx + 8 * 3]

hlt
