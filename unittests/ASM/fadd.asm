%ifdef CONFIG
{
  "Match": "All",
  "RegData": {
    "MM7": ["0xc90fdb0000000000", "0x4001"]
  }
}
%endif

; calcuate pi + pi
fld dword [rel pi]
fld dword [rel pi]
faddp

hlt

align 8
pi:     dd 0x40490fdb ; 3.14...
one:    dd 0x3f800000 ; 1.0
ptone:  dd 0x3dcccccd ; 0.1
