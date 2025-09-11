%ifdef CONFIG
{
  "MemoryRegions": {
    "0x100000000": "4096"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; FEX-Emu had a bug where x87 loadstores at a page boundary with reduced precision enabled would loadstore 128-bits.

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

; Do a BCD store
fbstp [rdx - 10]

; Regular 80-bit store
fstp tword [rdx - 10]

hlt
