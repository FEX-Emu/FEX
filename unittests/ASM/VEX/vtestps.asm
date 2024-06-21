%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "R15":  "0x000000000003B77F",
    "R14":  "0x000000000003B77F",
    "XMM0": ["0x0000000000000000", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM1": ["0xFFFFFFFFFFFFFFFF", "0xFFFFFFFFFFFFFFFF", "0xFFFFFFFFFFFFFFFF", "0xFFFFFFFFFFFFFFFF"],
    "XMM2": ["0x4142434445464748", "0x5152535455565758", "0x4142434445464748", "0x5152535455565758"]
  }
}
%endif

; Uses AX and BX and stores result in the passed in register
; CF:ZF
%macro zfcfmerge 1
  lahf

  ; Shift CF to zero
  shr ax, 8

  ; Move to a temp
  mov bx, ax
  and rbx, 1

  shl %{1}, 1
  or %{1}, rbx

  shl %{1}, 1

  ; Move to a temp
  mov bx, ax

  ; Extract ZF
  shr bx, 6
  and rbx, 1

  ; Insert ZF
  or %{1}, rbx
%endmacro

%macro tests 2
  vtestps %{1}0, [rdx + 32 * 3]
  zfcfmerge %{2}
  vtestps %{1}1, [rdx + 32 * 4]
  zfcfmerge %{2}
  vtestps %{1}2, [rdx + 32 * 5]
  zfcfmerge %{2}
  vtestps %{1}0, [rdx + 32 * 6]
  zfcfmerge %{2}
  vtestps %{1}1, [rdx + 32 * 7]
  zfcfmerge %{2}
  vtestps %{1}2, [rdx + 32 * 8]
  zfcfmerge %{2}
  vtestps %{1}0, [rdx + 32 * 9]
  zfcfmerge %{2}
  vtestps %{1}1, [rdx + 32 * 10]
  zfcfmerge %{2}
  vtestps %{1}2, [rdx + 32 * 11]
  zfcfmerge %{2}
%endmacro

lea rdx, [rel .data]

mov rax, 0
mov rbx, 0
mov r15, 0
mov r14, 0

vmovaps ymm0, [rdx + 32 * 0]
vmovaps ymm1, [rdx + 32 * 1]
vmovaps ymm2, [rdx + 32 * 2]

; Accumulate xmm results in r15
tests xmm, r15
; Accumulate ymm results in r14
tests ymm, r14

hlt

align 32
.data:
dq 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000
dq 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF
dq 0x4142434445464748, 0x5152535455565758, 0x4142434445464748, 0x5152535455565758

; Match
dq 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000
dq 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF
dq 0x4142434445464748, 0x5152535455565758, 0x4142434445464748, 0x5152535455565758

; Match on not
dq 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF
dq 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000
dq 0xBEBDBCBBBAB9B8B7, 0xAEADACABAAA9A8A7, 0xBEBDBCBBBAB9B8B7, 0xAEADACABAAA9A8A7

; No match on either case
dq 1, 1, 1, 1
dq 2, 2, 2, 2
dq 3, 3, 3, 3
