%ifdef CONFIG
{
  "RegData": {
    "MM0":  "0x3f800000bf800000",
    "MM1":  "0x43000000c3000000",
    "MM2":  "0xc700000046fffe00",
    "MM3":  "0x0" 
  },
  "HostFeatures": ["3DNOW"]
}
%endif

pi2fw mm0, [rel data1]
pi2fw mm1, [rel data2]
pi2fw mm2, [rel data3]
pi2fw mm3, [rel data4]

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
