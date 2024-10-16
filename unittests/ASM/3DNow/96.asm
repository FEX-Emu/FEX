%ifdef CONFIG
{
  "RegData": {
    "MM0": "0xbf800000bf800000",
    "MM1": "0xbc000000bc000000",
    "MM2": "0x3f8000003f800000"
  },
  "HostFeatures": ["3DNOW"]
}
%endif

pfrcp mm0, [rel data1]
pfrcp mm1, [rel data2]
pfrcp mm2, [rel data3]

hlt

align 8
data1:
dd -1.0
dd 1.0

data2:
dd -128.0
dd 128.0

data3:
dd 1.0
dd -1.0
