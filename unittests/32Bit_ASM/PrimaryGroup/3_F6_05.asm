%ifdef CONFIG
{
  "RegData": {
    "RDI": "0x00000000000003fc"
  },
  "Mode": "32BIT"
}
%endif

%macro ofcfmerge 0
  ; Get CF
  setc al
  ; Get OF
  seto bl
  and eax, 1
  and ebx, 1

  ; Merge in to results
  shl edi, 1
  or edi, eax

  ; Merge in to results
  shl edi, 1
  or edi, ebx
%endmacro

mov edi, 0

; Max Negative
mov al, 0x80
mov bl, 0x80

imul bl

ofcfmerge

; Max Positive
mov al, 0x79
mov bl, 0x79

imul bl

ofcfmerge

; Max Positive and Max Negative
mov al, 0x79
mov bl, 0x80

imul bl

ofcfmerge

; Max Positive and Max Negative
mov al, 0x80
mov bl, 0x79

imul bl

ofcfmerge

; No Overflow

mov al, 0x1
mov bl, 0x1

imul bl

ofcfmerge

hlt
