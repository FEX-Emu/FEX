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
mov eax, 0x80000000
mov ebx, 0x80000000

imul ebx

ofcfmerge

; Max Positive
mov eax, 0x7FFFFFFF
mov ebx, 0x7FFFFFFF

imul ebx

ofcfmerge

; Max Positive and Max Negative
mov eax, 0x7FFFFFFF
mov ebx, 0x80000000

imul ebx

ofcfmerge

; Max Positive and Max Negative
mov eax, 0x80000000
mov ebx, 0x7FFFFFFF

imul ebx

ofcfmerge

; No Overflow

mov eax, 0x1
mov ebx, 0x1

imul ebx

ofcfmerge

hlt
