%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0600"
  }
}
%endif

mov rax, 0x4142434445464847
test al, 0x61

mov rax, 0
lahf

hlt
