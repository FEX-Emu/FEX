%ifdef CONFIG
{
  "Match": "All",
  "RegData": {
    "RAX": "0x1"
  }
}
%endif

mov esi, 50

.jump_start:
mov edi, 1
test edi, edi
nop
nop
nop
nop
nop
nop
nop
nop

jz .local
mov eax, 1
jmp .end

.local:
mov eax, 0

.end:
sub esi, 1
test esi, esi
jz .jump_start
hlt
