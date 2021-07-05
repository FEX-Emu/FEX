%ifdef CONFIG
{
  "RegData": {
    "RDI": "0x00000000000003fc"
  }
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
mov ax, 0x8000
mov bx, 0x8000

imul bx

ofcfmerge

; Max Positive
mov ax, 0x7FFF
mov bx, 0x7FFF

imul bx

ofcfmerge

; Max Positive and Max Negative
mov ax, 0x7FFF
mov bx, 0x8000

imul bx

ofcfmerge

; Max Positive and Max Negative
mov ax, 0x8000
mov bx, 0x7FFF

imul bx

ofcfmerge

; No Overflow

mov ax, 0x1
mov bx, 0x1

imul bx

ofcfmerge

hlt
