%ifdef CONFIG
{
  "RegData": {
    "MM0":  "0x3f800000bf800000",
    "MM1":  "0x43000000c3000000",
    "MM2":  "0xbf8000003f800000",
    "MM3":  "0x0"
  },
  "HostFeatures": ["3DNOW"]
}
%endif

pi2fd mm0, [rel data1]
pi2fd mm1, [rel data2]
pi2fd mm2, [rel data3]
pi2fd mm3, [rel data4]

hlt

align 8
data1:
dd -1
dd 1

data2:
dd -128
dd 128

data3:
dd 1
dd -1

data4:
dd 0x0
dd 0x0
