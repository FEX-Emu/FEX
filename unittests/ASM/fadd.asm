%ifdef CONFIG
{
  "Match": "All",
  "RegData": {
    "MM0": ["0x0000000000000000", "0x0000"],
    "MM1": ["0x0000000000000000", "0x0000"],
    "MM2": ["0x0000000000000000", "0x0000"],
    "MM3": ["0x0000000000000000", "0x0000"],
    "MM4": ["0x0000000000000000", "0x0000"],
    "MM5": ["0x0000000000000000", "0x0000"],
    "MM6": ["0xc90fdb0000000000", "0x4000"],
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