%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1200",
    "RDI": "0xE000000A",
    "RSI": "0xE0000002"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x6162
mov [rdx + 8 * 0], rax
mov rax, 0x4546
mov [rdx + 8 * 1], rax

lea rdi, [rdx + 8 * 1]
lea rsi, [rdx + 8 * 0]

cld
cmpsw ; rdi cmp rsi
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
