%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x3f8000003f800000",
    "MM1": "0x3f8000003f800000",
    "MM2": "0x00000000bf800000",
    "MM3": "0x3f8000003f800000"
  },
  "HostFeatures": ["3DNOW"]
}
%endif

movq mm0, [rel data5]
movq mm1, [rel data6]
movq mm2, [rel data7]
movq mm3, [rel data8]

pfmax mm0, [rel data1]
pfmax mm1, [rel data2]
pfmax mm2, [rel data3]
pfmax mm3, [rel data4]

hlt

align 8
data1:
dd 1.0
dd 1.0

data2:
dd 1.0
dd 1.0

data3:
dd -1.0
dd 0.0

data4:
dd 0.0
dd 1.0

data5:
dd 0.0
dd 0.0

data6:
dd 0.0
dd 1.0

data7:
dd -1.0
dd 0.0

data8:
dd 1.0
dd 0.0
