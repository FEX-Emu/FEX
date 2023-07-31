%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000000008601"
  }
}
%endif

; FEX had a bug where OF for SHRD wasn't getting calculated correctly.
; OF with SHRD set if the sign bit has changed.
; FEX /previously/ calculated it like regular SHR, where it contained the original MSB.

mov edi, 0x35b292fc
mov ebp, 0x37d434ad
shrd edi, ebp, 1

mov rax, 0

lahf
; Load OF
seto al

; Mask out AF, SHRD leaves it undefined
and rax, 0xEFFF

hlt
