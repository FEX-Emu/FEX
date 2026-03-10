%ifdef CONFIG
{
  "Match": "All",
  "RegData": {
    "RDI": "0xE8000020",
    "R11": "0xDAD10"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

; Data we want to store
mov rax, 0xDEADBEEFBAD0DAD1

; Starting address to store to
mov rdi, 0xe8000000

; How many elements we want to store
mov rcx, 0x10

; Direction to increment (Increment when cleared)
cld

; Store bytes
repne stosw

mov r11, 0
mov r10, 0xe8000000

movzx r12, word [r10 + 0]
add r11, r12
movzx r12, word [r10 + 2]
add r11, r12
movzx r12, word [r10 + 4]
add r11, r12
movzx r12, word [r10 + 6]
add r11, r12
movzx r12, word [r10 + 8]
add r11, r12
movzx r12, word [r10 + 10]
add r11, r12
movzx r12, word [r10 + 12]
add r11, r12
movzx r12, word [r10 + 14]
add r11, r12
movzx r12, word [r10 + 16]
add r11, r12
movzx r12, word [r10 + 18]
add r11, r12
movzx r12, word [r10 + 20]
add r11, r12
movzx r12, word [r10 + 22]
add r11, r12
movzx r12, word [r10 + 24]
add r11, r12
movzx r12, word [r10 + 26]
add r11, r12
movzx r12, word [r10 + 28]
add r11, r12
movzx r12, word [r10 + 30]
add r11, r12

hlt
