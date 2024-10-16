%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x00000001FFFFFFFF",
    "MM1": "0x00000080FFFFFF80",
    "MM2": "0xFFFF800000007FFF",
    "MM3": "0x0"
  },
  "HostFeatures": ["3DNOW"]
}
%endif

pi2fw mm0, [rel data1]
pi2fw mm1, [rel data2]
pi2fw mm2, [rel data3]
pi2fw mm3, [rel data4]

pf2iw mm0, mm0
pf2iw mm1, mm1
pf2iw mm2, mm2
pf2iw mm3, mm3

hlt

align 8
data1:
dw -1
dw 0xFF
dw 1
dw 0xFF

data2:
dw -128
dw 0xFFFF
dw 128
dw 0xFFFF

data3:
dw 0x7FFF
dw 0x4242
dw 0x8000
dw 0x5252

data4:
dw 0x0
dw 0x1
dw 0x0
dw 0x2
