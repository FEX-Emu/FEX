%ifdef CONFIG
{
  "RegData": {
    "RAX":  "1",
    "RBX":  "0",
    "MM0":  "0x40600000",
    "MM1":  "0x40500000",
    "MM2":  "0x40400000",
    "MM3":  "0x40300000",
    "MM4":  "0x40200000",
    "MM5":  "0x40000000",
    "MM6":  "0x3ff00000",
    "MM7":  "0x40700000"
  },
  "Mode": "32BIT"
}
%endif

; Set the stack with different values.
; Then do fincstp and store the stack values into MMX registers through memory
; such that MM0 has the value of ST0 and so on.

mov eax, 0x3ff00000 ; 1.0
mov [rel temp], eax
fld dword [rel temp]

mov eax, 0x40000000 ; 2.0
mov [rel temp], eax
fld dword [rel temp]

mov eax, 0x40200000 ; 4.0
mov [rel temp], eax
fld dword [rel temp]

mov eax, 0x40300000
mov [rel temp], eax
fld dword [rel temp]

mov eax, 0x40400000
mov [rel temp], eax
fld dword [rel temp]

mov eax, 0x40500000
mov [rel temp], eax
fld dword [rel temp]

mov eax, 0x40600000
mov [rel temp], eax
fld dword [rel temp]

mov eax, 0x40700000
mov [rel temp], eax
fld dword [rel temp]

; Store top in RBX
xor eax, eax
fnstsw ax
shr ax, 11
and ax, 7
mov bx, ax

; Move the value of stop
; ST0 is currently 0x4070000000000000
fincstp

; Store top in eax
xor eax, eax
fnstsw ax
shr ax, 11
and ax, 7

; Now ST0 is 0x4060000000000000
fstp dword [rel stack + 8 * 0]
fstp dword [rel stack + 8 * 1]
fstp dword [rel stack + 8 * 2]
fstp dword [rel stack + 8 * 3]
fstp dword [rel stack + 8 * 4]
fstp dword [rel stack + 8 * 5]
fstp dword [rel stack + 8 * 6]
fstp dword [rel stack + 8 * 7]

movq mm0, [rel stack + 8 * 0]
movq mm1, [rel stack + 8 * 1]
movq mm2, [rel stack + 8 * 2]
movq mm3, [rel stack + 8 * 3]
movq mm4, [rel stack + 8 * 4]
movq mm5, [rel stack + 8 * 5]
movq mm6, [rel stack + 8 * 6]
movq mm7, [rel stack + 8 * 7]

hlt

align 4096
temp: dq 0
stack: times 8 dq 0
