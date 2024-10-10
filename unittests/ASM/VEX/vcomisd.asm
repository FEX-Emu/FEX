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
vcomisd xmm0, [rdx + 16 * 1] ; 1.0 <comp> 4.0
; 0: CF - 00000001
; 1:    - 00000010
; 2: PF - 00000100
; 3:  0 - 00000000
; 4: AF - 00000000 <- 0
; 5:  0 - 00000000
; 6: ZF - 00000000
; 7: SF - 00000000 <- 0
; ================
;         00000111
; OF: LAHF doesn't load - 0

mov rax, 0
lahf
mov rbx, rax

vcomisd xmm0, [rdx + 16 * 2] ; 1.0 <comp> NaN
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
dq 0x3FF0000000000000
dq 0x4000000000000000

dq 0x4010000000000000
dq 0x4010000000000000

dq 0x7FF8000000000000
dq 0x4010000000000000
