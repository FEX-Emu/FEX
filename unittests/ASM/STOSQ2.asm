%ifdef CONFIG
{
  "Match": "All",
  "RegData": {
    "RAX": "0xDEADBEEFBAD0DAD1"
  },

  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

; Starting address to store to
mov rdi, 0xe8000000
; Store value
mov rax, 0xDEADBEEFBAD0DAD1
mov [rdi], rax

; Set counter to zero
mov ecx, 0
; Set store value to zero
mov rax, 0

rep STOSQ

; Reload what we just stored
; Ensure that STOSQ didn't write
mov rdi, 0xe8000000
mov rax, [rdi]

hlt
