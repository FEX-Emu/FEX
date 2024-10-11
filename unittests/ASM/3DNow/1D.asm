%ifdef CONFIG
{
  "RegData": {
    "MM0":  "0x00000001FFFFFFFF",
    "MM1":  "0x00000080FFFFFF80",
    "MM2":  "0xFFFFFFFF00000001",
    "MM3":  "0x0"
  },
  "HostFeatures": ["3DNOW"]
}
%endif

pi2fd mm0, [rel data1]
pi2fd mm1, [rel data2]
pi2fd mm2, [rel data3]
pi2fd mm3, [rel data4]

pf2id mm0, mm0
pf2id mm1, mm1
pf2id mm2, mm2
pf2id mm3, mm3

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
