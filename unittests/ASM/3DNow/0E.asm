%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFF"
  },
  "HostFeatures": ["3DNOW"]
}
%endif

; Load all x87 registers
finit
fldz
fldz
fldz
fldz
fldz
fldz
fldz

; femms sets all the tag bits to 0b11
femms

mov rdx, 0xe0000000
o32 fstenv [rdx]

mov eax, 0
mov ax, word [rdx + 8] ; Offset 8 in the structure has FTW

hlt
