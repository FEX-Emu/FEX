%ifdef CONFIG
{
  "RegData": {
    "MM0":  "0x3f8000003f800000",
    "MM1":  "0x3f0000003f000000",
    "MM2":  "0x3eaaaaab3eaaaaab"
  },
  "HostFeatures": ["3DNOW"]
}
%endif

pfrsqrt mm0, [rel data1]
pfrsqrt mm1, [rel data2]
pfrsqrt mm2, [rel data3]

hlt

align 8
data1:
dd 1.0
dd 16.0

data2:
dd 4.0
dd 25.0

data3:
dd 9.0
dd 1.0
