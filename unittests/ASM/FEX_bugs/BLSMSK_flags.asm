%ifdef CONFIG
{
  "RegData": {
      "RDX": "0xcafe",
      "RBX": "5"
  },
  "HostFeatures": ["BMI1"]
}
%endif

; Result in all-1's due to underflow so SF/CF set
mov rbx, 1
mov rax, 0
blsmsk rax, rax

jns fexi_fexi_im_so_broken
jz fexi_fexi_im_so_broken
jnc fexi_fexi_im_so_broken
jo fexi_fexi_im_so_broken

; Result in 1, so all flags clear
mov rbx, 2
mov rax, 11
blsmsk rax, rax

js fexi_fexi_im_so_broken
jz fexi_fexi_im_so_broken
jc fexi_fexi_im_so_broken
jo fexi_fexi_im_so_broken

; Result in all-1's without carry, so SF set
mov rbx, 3
mov rax, 0x8000000000000000
blsmsk rax, rax

jns fexi_fexi_im_so_broken
jz fexi_fexi_im_so_broken
jc fexi_fexi_im_so_broken
jo fexi_fexi_im_so_broken

; Make sure we're correctly clearing the zero flag
mov rbx, 4
mov rax, 0
add rax, rax
jnz fexi_fexi_im_so_broken
blsmsk rax, rax
jz fexi_fexi_im_so_broken

; Make sure we're correctly clearing the overflow flag
mov rbx, 5
mov al, 0x7F
inc al
jno fexi_fexi_im_so_broken
blsmsk rax, rax
jo fexi_fexi_im_so_broken

; Happy ending
mov rdx, 0xcafe
hlt

fexi_fexi_im_so_broken:
mov rdx, 0xdead
hlt
