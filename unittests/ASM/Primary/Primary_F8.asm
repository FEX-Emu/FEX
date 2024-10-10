%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0"
  }
}
%endif

clc

; Get CF
sbb rax, rax
and rax, 1

hlt
