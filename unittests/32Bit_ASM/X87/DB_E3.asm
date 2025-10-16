%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x037F"
  },
  "Mode": "32BIT"
}
%endif

fninit

; Ensures that fnstcw after fninit sets the correct value
fnstcw [rel control]
mov ax, word [rel control]

hlt

align 4096
control:
times 2 db 0 ; Reserve space for the FPU control word
