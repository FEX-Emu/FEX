%ifdef CONFIG
{
  "RegData": {
      "RDX": "0xcafe"
  },
  "HostFeatures": ["BMI1"]
}
%endif

; Source 0 sets CF and ZF
mov rax, 0
blsr rax, rax

js fexi_fexi_im_so_broken
jnz fexi_fexi_im_so_broken
jnc fexi_fexi_im_so_broken
jo fexi_fexi_im_so_broken

; Source 1 sets ZF
mov rax, 1
blsr rax, rax

js fexi_fexi_im_so_broken
jnz fexi_fexi_im_so_broken
jc fexi_fexi_im_so_broken
jo fexi_fexi_im_so_broken

; Source 3 sets nothing
mov rax, 3
blsr rax, rax

js fexi_fexi_im_so_broken
jz fexi_fexi_im_so_broken
jc fexi_fexi_im_so_broken
jo fexi_fexi_im_so_broken

; Source all-1's sets SF
mov rax, 0xffffffffffffffff
blsr rax, rax

jns fexi_fexi_im_so_broken
jz fexi_fexi_im_so_broken
jc fexi_fexi_im_so_broken
jo fexi_fexi_im_so_broken

; Make sure we're correctly clearing the overflow flag
mov rbx, 5
mov al, 0x7F
inc al
jno fexi_fexi_im_so_broken
blsr rax, rax
jo fexi_fexi_im_so_broken

; Happy ending
mov rdx, 0xcafe
hlt

fexi_fexi_im_so_broken:
mov rdx, 0xdead
hlt
