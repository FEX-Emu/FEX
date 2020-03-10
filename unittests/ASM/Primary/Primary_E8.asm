%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1",
    "RBX": "0x1"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rsp, 0xe0000020
mov rax, 0
mov rbx, 0

; NASM doesn't have a way to explicitly encode a 16bit or 32bit relative call
; Manually encode
db 0x66 ; 16bit
db 0xE8 ; CALL
db 0x02 ; +0x0002 (Just past the next JUMP instruction
db 0x00

jmp .end1

.target1:
mov rax, 1
ret

.end1:

; NASM doesn't have a way to explicitly encode a 16bit or 32bit relative call
; Manually encode
db 0xE8 ; CALL
db 0x02 ; +0x00000002 (Just past the next JUMP instruction
db 0x00
db 0x00
db 0x00

jmp .end2

.target2:
mov rbx, 1
ret

.end2:
hlt
