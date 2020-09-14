%ifdef CONFIG
{
  "RegData": {
    "MM0": ["0x0000000045464748", "0x0"],
    "MM1": ["0x5152535455565758", "0x0"]
  }
}
%endif

mov rax, 0x4142434445464748
mov rbx, 0x5152535455565758

movd mm0, eax
movq mm1, rbx
hlt
