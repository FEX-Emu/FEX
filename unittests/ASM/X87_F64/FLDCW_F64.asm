%ifdef CONFIG
{
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" },
  "RegData": {
    "RAX": "0x3",
    "RBX": "0x2"
  }
}
%endif

lea rbp, [rel data]
mov rdx, 0xe0000000
mov rcx, 0xe0004000

; save fcw
fnstcw [rdx]
; set rounding to truncate
mov eax, 0
mov ax, [rdx]
or ah, 0xc
mov [rdx+8], ax
fldcw [rdx+8]

fld dword [rbp]
fistp dword [rdx+16]
mov ebx, [rdx+16]

; restore fcw
fldcw [rdx]
fld dword [rbp]
fistp dword[rdx+16]
mov eax, [rdx+16]

hlt

align 8
data:
   dd 0x40266666 ; 2.6
