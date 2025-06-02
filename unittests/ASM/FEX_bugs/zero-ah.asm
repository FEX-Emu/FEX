%ifdef CONFIG
{
  "RegData": {
    "RCX": "0x0"
  }
}
%endif

; FEX had a bug where `mov ah, 0` and `xor ah, ah` would zero the wrong register
; subpart.

mov al, 127
mov ah, 234
mov ah, 0

cmp al, 127
jne fexi_fexi_im_so_broken
cmp ah, 0
jne fexi_fexi_im_so_broken

mov al, 127
mov ah, 234
xor ah, ah

cmp al, 127
jne fexi_fexi_im_so_broken
cmp ah, 0
jne fexi_fexi_im_so_broken

mov ecx, 0
hlt

fexi_fexi_im_so_broken:
mov ecx, 0xdeadbeef
hlt
