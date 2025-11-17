%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1200",
    "RDI": "0xE000000A",
    "RSI": "0xE0000002"
  },
  "Mode": "32BIT"
}
%endif

mov edx, 0xe0000000

mov eax, 0x6162
mov [edx + 8 * 0], eax
mov eax, 0x4546
mov [edx + 8 * 1], eax

lea edi, [edx + 8 * 1]
lea esi, [edx + 8 * 0]

cld
cmpsw
; cmp = 0x4546 - 0x6162 = 0xE3E4
; 0: CF - 00000001
; 1:    - 00000010
; 2: PF - 00000100
; 3:  0 - 00000000
; 4: AF - 00000000
; 5:  0 - 00000000
; 6: ZF - 00000000
; 7: SF - 10000000
; ================
;         10000111
; OF: LAHF doesn't load - 0

mov eax, 0
lahf

hlt
