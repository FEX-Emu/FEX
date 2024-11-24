%ifdef CONFIG
{
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

; Load the data in to an x87 register for storing.
fld tword [rdx - 16]
fld tword [rdx - 16]

; Do an 80-bit store at the edge of a page.
; Ensuring tword stores don't extend past the end of a page.
; If storing past the end of the page, then an unhandled SIGSEGV will occur.
fstp tword [rdx - 10]

; Do an 80-bit bcd store at the edge of a page.
fbstp [rdx - 10]

hlt
