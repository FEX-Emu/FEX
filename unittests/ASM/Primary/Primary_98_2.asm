%ifdef CONFIG
{
  "RegData": {
    "R10": "0xFFFFFFFF80000001",
    "R11": "0x00000000FFFF8001",
    "R12": "0x414243444546FF81",
    "R13": "0x0000000000000001",
    "R14": "0x0000000000000001",
    "R15": "0x4142434445460001"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

; Positive 8bit
mov rax, 0x4142434445464701
cbw
mov r15, rax

; Positive 16bit
mov rax, 0x4142434445460001
cwde
mov r14, rax

; Positive 32bit
mov rax, 0x4142434400000001
cdqe
mov r13, rax

; Negative 8bit
mov rax, 0x4142434445464781
cbw
mov r12, rax

; Negative 16bit
mov rax, 0x4142434445468001
cwde
mov r11, rax

; Negative 32bit
mov rax, 0x4142434480000001
cdqe
mov r10, rax

hlt
