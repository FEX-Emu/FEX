%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x414243444F800000", "0x5152535455565758"]
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

movapd xmm0, [rdx + 8 * 0]

; Ensures that a large "negative" 32bit value converts correctly in cvtsi2ss when treated as a 64bit value
; Upper bits being zero
mov rax, 0xFFFFFFFF
cvtsi2ss xmm0, rax

hlt
