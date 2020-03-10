%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x8700",
    "RBX": "0x8300",
    "RCX": "0x0200",
    "RSI": "0x0300"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax
mov rax, 0x6162636465666768
mov [rdx + 8 * 2], rax
mov rax, 0x6162636465666768
mov [rdx + 8 * 3], rax

mov rax, -256
cmp qword [rdx + 8 * 3 + 0], rax
; cmp = 0x6162636465666768 - -256(0xFFFFFFFFFFFF00) = 0x6162636465666512
; 0: CF - 00000001
; 1:    - 00000010
; 2: PF - 00000000
; 3:  0 - 00000000
; 4: AF - 00000000
; 5:  0 - 00000000
; 6: ZF - 00000000
; 7: SF - 00000000
; ================
;         00000011
; OF: LAHF doesn't load - 0
mov rax, 0
lahf
mov rsi, rax

mov rax, 0x61626364
cmp qword [rdx + 8 * 2 + 0], rax
; cmp = 0x6162636465666768- 0x61626364 = 0x6162636404040404
; 0: CF - 00000000
; 1:    - 00000010
; 2: PF - 00000000
; 3:  0 - 00000000
; 4: AF - 00000000
; 5:  0 - 00000000
; 6: ZF - 00000000
; 7: SF - 00000000
; ================
;         00000010
; OF: LAHF doesn't load - 0
mov rax, 0
lahf
mov rcx, rax

mov rax, 0x61626364
cmp dword [rdx + 8 * 1 + 0], eax
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
mov rax, 0
lahf
mov rbx, rax

mov rax, 0x6162
cmp word [rdx + 8 * 0 + 2], ax
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
mov rax, 0
lahf

hlt
