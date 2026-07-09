%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000003ce0950",
     "XMM0": [
      "0x04a6e5eb4c738254",
      "0xc7f69988a166b329", 
      "0xf47e815854c72b11", 
      "0x77d12321b16b61ad" 
    ]
  }
}
%endif

lea rdx, [rel .data]
vmovdqa ymm0, [rdx]

mov rax, 0xDEADBEEFCAFEBABE

o16 cvttss2si eax, XMM0

hlt


align 16
.data:
dq 0x04a6e5eb4c738254, 0xc7f69988a166b329, 0xf47e815854c72b11, 0x77d12321b16b61ad