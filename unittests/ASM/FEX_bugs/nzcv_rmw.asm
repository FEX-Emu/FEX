%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xcafe"
  }
}
%endif

; FEX had a bug where an NZCV RMW would fail to calculate previously deferred
; flags, resulting in garbage flag values

; First zero NZCV and break visibility
mov rax, 0
add rax, 1
jz fexi_fexi_im_so_broken

jmp .begin
.begin:

; NZCV is zero. Set it to something nonzero with a deferred flag operation.
mov rax, 0
popcnt rax, rax

; Now do a variable shift that preserves flags. This would clear ZF if not for
; the condition on the shift flags.
mov rbx, 100
mov cl, 0
sar rbx, cl

; ZF should still be set.
jnz fexi_fexi_im_so_broken

mov rax, 0xcafe
hlt

fexi_fexi_im_so_broken:
mov rax, 0xdead
hlt
