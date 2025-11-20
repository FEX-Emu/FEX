%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1200",
    "RDI": "0xE0000001"
  },
  "Mode": "32BIT"
}
%endif

mov edx, 0xe0000000

mov eax, 0x45464748
mov [edx + 8 * 0], eax
mov eax, 0x41424344
mov [edx + 8 * 0 + 4], eax
mov eax, 0x55565758
mov [edx + 8 * 1], eax
mov eax, 0x51525354
mov [edx + 8 * 1 + 4], eax
mov eax, 0x0
mov [edx + 8 * 2], eax

lea edi, [edx + 8 * 0]

cld
mov eax, 0x61
scasb
; cmp = 0x61 - 0x48 = 0x19
; 0: CF - 00000000
; 1:    - 00000010
; 2: PF - 00000000
; 3:  0 - 00000000
; 4: AF - 00010000
; 5:  0 - 00000000
; 6: ZF - 00000000
; 7: SF - 00000000
; ================
;         00010010
; OF: LAHF doesn't load - 0

mov eax, 0
lahf

hlt
