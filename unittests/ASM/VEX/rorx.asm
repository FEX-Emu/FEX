%ifdef CONFIG
{
  "RegData": {
      "RAX": "0x8000000000000000",
      "RBX": "0xFF",
      "RCX": "0xF00000000000000F",
      "RDX": "0x80000000",
      "RSI": "0xFF",
      "RDI": "0xF000000F",
      "R8":  "0",
      "R9": "0x0000000045464748",
      "R10": "0x0000000022a323a4"

  },
  "HostFeatures": ["BMI2"]
}
%endif

; Trivial test
mov rax, 1
rorx rax, rax, 1

; More than one bit
mov rbx, 0xFF
rorx rcx, rbx, 4

; Test that we mask the rotation amount above the operand size (should leave rcx's value alone).
rorx rcx, rcx, 64

; 32-bit

; Trivial test
mov edx, 1
rorx edx, edx, 1

; More than one bit
mov esi, 0xFF
rorx edi, esi, 4,

; Test that we mask the rotation amount above the operand size (should leave edi's value alone).
rorx edi, edi, 32

; Zero-extending behavior
mov r8, 0xFFFFFFFF00000000
rorx r8d, r8d, 0

mov r9, 0x4142434445464748
rorx r9d, r9d, 0xE0

mov r10, 0x4142434445464748
rorx r10d, r10d, 0xE1

hlt
