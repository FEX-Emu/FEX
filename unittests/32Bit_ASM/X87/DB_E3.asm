%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x037F"
  },
  "Mode": "32BIT"
}
%endif

mov eax, 0
mov esp, 0xe0000008
fninit

; Ensures that fnstcw after fninit sets the correct value
fnstcw [esp]
mov ax, word [esp]

hlt
