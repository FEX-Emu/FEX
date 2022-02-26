%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x400000003f800000", "0x4080000040400000"]
  }
}
%endif

vcvtph2ps xmm0, [rel data1]

hlt
align 16

data1:
dw 1.0
dw 2.0
dw 3.0
dw 4.0
