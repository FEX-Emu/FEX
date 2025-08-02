%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x037F"
  }
}
%endif

section .bss
control: resb 2 ; Reserve space for the FPU control word

section .text
global _start
_start:
fninit

; Ensures that fnstcw after fninit sets the correct value
fnstcw [rel control]
mov ax, word [rel control]

hlt
