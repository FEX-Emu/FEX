%ifdef CONFIG
{
  "Mode": "32BIT"
}
%endif

mov ecx, 0x10
.loop:
dec ecx
test ecx, ecx
jnz .loop
.end:

mov ecx, 0x10
.loop2:
dec ecx
test ecx, ecx
jz .end2
jmp .loop2

.end2:
hlt