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

; NASM doesn't have a way to explicitly encode a 16bit or 32bit relative jump
; Manually encode
db 0x66 ; 16bit
db 0xE9 ; JMP
db 0x02 ; +0x0002 (Just past the next JUMP instruction
db 0x00

.back1:
jmp .end1

.target1:
mov rax, 1
jmp .back1

.end1:

; NASM doesn't have a way to explicitly encode a 16bit or 32bit relative jump
; Manually encode
db 0xE9 ; JMP
db 0x02 ; +0x00000002 (Just past the next JUMP instruction
db 0x00
db 0x00
db 0x00

.back2:
jmp .end2

.target2:
mov rbx, 1
jmp .back2

.end2:
hlt
