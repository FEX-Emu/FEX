%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000cafecafe"
  }
}
%endif

; This test checks for proper behaviour of the parity flag. Older FEX versions
; would accidentally turn JP into a zero/nonzero check of the result.

; rax = 0x20, odd parity
mov rax, 0x10
mov rbx, 0x10
add rax, rbx
jpe fexi_fexi_im_so_broken

; rax = 0x32, odd parity
mov rax, 0x10
mov rbx, 0x22
xor rax, rbx
jpe fexi_fexi_im_so_broken

; rax = 0x41, even parity
mov rax, 0x40
mov rbx, 0x01
or rax, rbx
jpo fexi_fexi_im_so_broken

; rax = 0x43, even parity
mov rax, 0x43
mov rbx, 0xfe
and rax, rbx
jpo fexi_fexi_im_so_broken

; success code
mov rax, 0xcafecafe
hlt

; failure, rax != 0xcafecafe
fexi_fexi_im_so_broken:
hlt
