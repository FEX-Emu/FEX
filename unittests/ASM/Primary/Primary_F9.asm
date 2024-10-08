%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1"
  }
}
%endif

stc

; Get CF
sbb rax, rax
and rax, 1

hlt
