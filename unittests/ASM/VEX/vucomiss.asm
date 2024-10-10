%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "RAX": "0x4700",
    "RBX": "0x0300"
  }
}
%endif

lea rdx, [rel .data]

vmovaps xmm0, [rdx + 16 * 0]
vucomiss xmm0, [rdx + 16 * 1]
; 0: CF - 00000001
; 1:    - 00000010
; 2: PF - 00000000
; 3:  0 - 00000000
; 4: AF - 00000000 <- 0
; 5:  0 - 00000000
; 6: ZF - 00000000
; 7: SF - 00000000 <- 0
; ================
;         00000011
; OF: LAHF doesn't load - 0

mov rax, 0
lahf
mov rbx, rax

vucomiss xmm0, [rdx + 16 * 2]
; 0: CF - 00000001
; 1:    - 00000010
; 2: PF - 00000100
; 3:  0 - 00000000
; 4: AF - 00000000 <- 0
; 5:  0 - 00000000
; 6: ZF - 01000000
; 7: SF - 00000000 <- 0
; ================
;         01000111
; OF: LAHF doesn't load - 0

mov rax, 0
lahf

hlt

align 16
.data:
dq 0x515253543F800000
dq 0x5152535440000000

dq 0x5152535440800000
dq 0x5152535440800000

dq 0x515253547FC00000
dq 0x5152535440800000
