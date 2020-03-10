%ifdef CONFIG
{
  "RegData": {
    "R15": "0x4142434445464748",
    "R14": "0x929496989A9C9EA0",
    "R13": "0x0000000045464748",
    "R12": "0x000000009A9C9EA0",
    "R11": "0x828486888A8C8E90",
    "R10": "0x565B60656A6F7478",
    "R9":  "0x41424344454647A9",
    "R8":  "0x92949698FBFF0204",
    "RSI": "0xFFFFFFFFFFFF0204"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov rdx, 0x5152535455565758

; SIB gives us `scale * index + base + offset`
; scale = constant {1, 2, 4, 8}
; Index = <Reg>
; Base = <Reg>
; Offset = Constant {imm8, imm32}
lea r15, [rax]
lea r14, [rax + rdx]

lea r13d, [eax]
lea r12d, [eax + edx]

lea r11, [2 * rax]
lea r10, [4 * rax + rdx]

lea r9, [rax + 0x61]
lea r8, [rax + rdx + 0x61626364]

mov rsi, -1
lea si, [rax + rdx + 0x61626364]

hlt
