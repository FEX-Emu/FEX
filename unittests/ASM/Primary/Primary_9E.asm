%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFFFFFFFFFFD7FF"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

; Set EFLAGS to known value with sahf
mov rax, -1
sahf

; Now load back
mov rax, -1
lahf
; 0: CF - 00000001
; 1:    - 00000010
; 2: PF - 00000100
; 3:  0 - 00000000
; 4: AF - 00010000
; 5:  0 - 00000000
; 6: ZF - 01000000
; 7: SF - 10000000
; ================
;         11010111
; OF: LAHF doesn't load - 0

hlt
