%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x8600"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0xFFFFFFFF00000000
mov [rdx + 8 * 0], rax

test qword [rdx + 8 * 0], -1
; test = 0x4748 & 0x7172 = 0x4140
; 0: CF - 00000000
; 1:    - 00000010
; 2: PF - 00000100
; 3:  0 - 00000000
; 4: AF - 00000000 <- Undefined
; 5:  0 - 00000000
; 6: ZF - 00000000
; 7: SF - 10000000
; ================
;         10000110
; OF: LAHF doesn't load - 0

mov rax, 0
lahf

hlt

