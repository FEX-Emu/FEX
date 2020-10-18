%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x41424344"
  },
  "Mode": "32BIT"
}
%endif

mov edi, 0xe0000000
lea esp, [edi + 8 * 4]

mov eax, 0x41424344
mov [edi + 8 * 0], eax
mov eax, 0x51525354
mov [edi + 8 * 1], eax

mov eax, 0
db 0xFF
db 0x15
dd .jmp_data
jmp .end

.call_tgt:
mov eax, [edi + 8 * 0]
ret

; Couple things that could catch failure
mov eax, 0
jmp .end
mov eax, 0

.end:
hlt

.jmp_data:
dd .call_tgt
