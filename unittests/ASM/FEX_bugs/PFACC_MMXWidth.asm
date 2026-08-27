%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x40e0000040400000"
  },
  "HostFeatures": ["3DNOW"]
}
%endif

femms
movq mm0, [rel .dst_value]
movq mm1, [rel .src_value]

pfacc mm0, mm1

hlt

align 8
.dst_value:
dd 0x3f800000 ; 1.0
dd 0x40000000 ; 2.0

.src_value:
dd 0x40400000 ; 3.0
dd 0x40800000 ; 4.0
