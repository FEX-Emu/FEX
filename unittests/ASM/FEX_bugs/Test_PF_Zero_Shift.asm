%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000cafecafe"
  }
}
%endif

; This test checks that PF is not modified with zero shifts

; First, some smoke tests: nonzero shifts should set PF as expected
; 0b111_1111_1000 has odd parity
mov eax, 0xff
mov cl, 0x3
shl eax, cl
jpe fexi_fexi_im_so_broken

; 0b11_1111_1100 has even parity
mov eax, 0xff
mov cl, 0x2
shl eax, cl
jpo fexi_fexi_im_so_broken

; At this point, parity is even
; So now test that PF is preserved across zero shifts, regardless of output parity.

mov cl, 0
mov eax, 0x0f
shl eax, cl
jpo fexi_fexi_im_so_broken

mov eax, 0x0e
shl eax, cl
jpo fexi_fexi_im_so_broken

mov eax, 0x1f
shr eax, cl
jpo fexi_fexi_im_so_broken

mov eax, 0x1e
shr eax, cl
jpo fexi_fexi_im_so_broken

mov eax, 0x2f
sal eax, cl
jpo fexi_fexi_im_so_broken

mov eax, 0x2e
sal eax, cl
jpo fexi_fexi_im_so_broken

mov eax, 0x3f
sar eax, cl
jpo fexi_fexi_im_so_broken

mov eax, 0x3e
sar eax, cl
jpo fexi_fexi_im_so_broken

; success code
mov rax, 0xcafecafe
hlt

; failure, rax != 0xcafecafe
fexi_fexi_im_so_broken:
hlt
