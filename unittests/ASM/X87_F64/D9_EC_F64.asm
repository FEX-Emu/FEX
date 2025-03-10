%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x3e9a209b"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rcx, 0xe0000000

fldlg2

fst dword [rcx] ; Can't compare 64-bit precision with host
mov eax, [rcx]

hlt
