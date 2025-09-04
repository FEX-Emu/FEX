%ifdef CONFIG
{
  "RegData": {
    "R15":  "0x000000000003a759",
    "XMM1": ["0", "0"],
    "XMM2": ["0xFFFFFFFFFFFFFFFF", "0xFFFFFFFFFFFFFFFF"],
    "XMM3": ["0x4142434445464748", "0x5152535455565758"],
    "XMM4": ["0", "0"],
    "XMM5": ["0xFFFFFFFFFFFFFFFF", "0xFFFFFFFFFFFFFFFF"],
    "XMM6": ["0x4142434445464748", "0x5152535455565758"],
    "XMM7": ["0", "0"],
    "XMM8": ["0xFFFFFFFFFFFFFFFF", "0xFFFFFFFFFFFFFFFF"],
    "XMM9": ["0x4142434445464748", "0x5152535455565758"]
  },
  "HostFeatures": ["SSE4.1"]
}
%endif

; Uses AX and BX and stores result in r15
; CF:ZF
%macro zfcfmerge 0
  lahf

  ; Shift CF to zero
  shr ax, 8

  ; Move to a temp
  mov bx, ax
  and rbx, 1

  shl r15, 1
  or r15, rbx

  shl r15, 1

  ; Move to a temp
  mov bx, ax

  ; Extract ZF
  shr bx, 6
  and rbx, 1

  ; Insert ZF
  or r15, rbx
%endmacro

lea rdx, [rel .data]

mov rax, 0
mov rbx, 0
mov r15, 0

movaps xmm1, [rdx + 16 * 0]
movaps xmm2, [rdx + 16 * 1]
movaps xmm3, [rdx + 16 * 2]
movaps xmm4, [rdx + 16 * 0]
movaps xmm5, [rdx + 16 * 1]
movaps xmm6, [rdx + 16 * 2]
movaps xmm7, [rdx + 16 * 0]
movaps xmm8, [rdx + 16 * 1]
movaps xmm9, [rdx + 16 * 2]


ptest xmm1, [rdx + 16 * 3]
zfcfmerge
ptest xmm2, [rdx + 16 * 4]
zfcfmerge
ptest xmm3, [rdx + 16 * 5]
zfcfmerge
ptest xmm4, [rdx + 16 * 6]
zfcfmerge
ptest xmm5, [rdx + 16 * 7]
zfcfmerge
ptest xmm6, [rdx + 16 * 8]
zfcfmerge
ptest xmm7, [rdx + 16 * 9]
zfcfmerge
ptest xmm8, [rdx + 16 * 10]
zfcfmerge
ptest xmm9, [rdx + 16 * 11]
zfcfmerge

hlt

align 16
.data:
dq 0, 0
dq 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF
dq 0x4142434445464748, 0x5152535455565758

; Match
dq 0, 0
dq 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF
dq 0x4142434445464748, 0x5152535455565758

; Match on not
dq 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF
dq 0, 0
dq 0xBEBDBCBBBAB9B8B7, 0xAEADACABAAA9A8A7

; No match on either case
dq 1, 1
dq 2, 2
dq 3, 3
