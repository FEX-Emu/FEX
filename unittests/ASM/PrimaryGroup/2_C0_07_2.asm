%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x8300",
    "RBX": "0xFFFFFFFFFFFFFFFE"
  }
}
%endif


mov rax, 0
mov rbx, 0xA142434445464748
sar rbx, 62
lahf
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


hlt
