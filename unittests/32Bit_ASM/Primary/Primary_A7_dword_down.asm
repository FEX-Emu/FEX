%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1600",
    "RDI": "0xE0000004",
    "RSI": "0xDFFFFFFC"
  },
  "Mode": "32BIT"
}
%endif

mov edx, 0xe0000000

mov eax, 0x61626364
mov [edx + 8 * 0], eax
mov eax, 0x55565758
mov [edx + 8 * 1], eax

lea edi, [edx + 8 * 1]
lea esi, [edx + 8 * 0]

std
cmpsd
; cmp = 0x55565758 - 0x61626364 = 0xF3F3F3F4
; 0: CF - 00000001
; 1:    - 00000010
; 2: PF - 00000000
; 3:  0 - 00000000
; 4: AF - 00000000
; 5:  0 - 00000000
; 6: ZF - 00000000
; 7: SF - 10000000
; ================
;         10000011
; OF: LAHF doesn't load - 0
mov eax, 0
lahf

hlt
