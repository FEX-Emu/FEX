%ifdef CONFIG
{
  "RegData": {
    "MM7":  ["0x5354555657584142", "0x0000000000005152"],
    "MM6":  ["0xe94de5eae34fc1c0", "0x0000000000004039"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

finit ; enters x87 state

mov rax, 0x100000000
mov rbx, 0x4142434445464748
mov rcx, 0x5152535455565758
mov rdx, (0x100000000 + 0x1000 - 16)

mov [rdx], rbx
mov [rdx + 8], rcx

mov rdx, 0x100000000 + 0x1000

; Do an 80-bit load at the edge of a page.
; Ensuring tword loads don't extend past the end of a page.
fld tword [rdx - 10]

; Do an 80-bit BCD load at the edge of a page.
fbld [rdx - 10]

hlt
