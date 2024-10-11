%ifdef CONFIG
{
  "RegData": {
    "MM0": "0xc3800000c3800000",
    "MM1": "0xc7800000c7800000"
  },
  "HostFeatures": ["3DNOW"]
}
%endif

movq mm0, [rel data1]
movq mm1, [rel data2]

pfmul mm0, [rel data3]
pfmul mm1, [rel data4]

hlt

align 8
data1:
dd -1.0
dd 1.0

data2:
dd -128.0
dd 128.0

data3:
dd 256.0
dd -256.0

data4:
dd 512.0
dd -512.0
