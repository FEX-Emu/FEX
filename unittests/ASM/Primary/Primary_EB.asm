%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rsp, 0xe0000020
mov rax, 0

; NASM doesn't have a way to explicitly encode a 16bit or 32bit relative jump
; Manually encode
db 0xEB ; JUMP
db 0x02 ; +0x02 (Just past the next JUMP instruction

.back:
jmp .end

.target:
mov rax, 1
jmp .back

.end:
hlt
