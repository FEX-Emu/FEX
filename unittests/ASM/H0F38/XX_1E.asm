%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x0",
    "MM1": "0x0000000100000001",
    "MM2": "0x0000000100000001",
    "MM3": "0x0000000100000000"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x0000000000000000
mov [rdx + 8 * 0], rax
mov rax, 0xFFFFFFFFFFFFFFFF
mov [rdx + 8 * 1], rax
mov rax, 0x0000000100000001
mov [rdx + 8 * 2], rax
mov rax, 0xFFFFFFFF00000000
mov [rdx + 8 * 3], rax

; Test with full zero
pabsd mm0, [rdx + 8 * 0]

; Test with full negative
pabsd mm1, [rdx + 8 * 1]

; Test with full positive
pabsd mm2, [rdx + 8 * 2]

; Test a mix
pabsd mm3, [rdx + 8 * 3]

hlt
