%ifdef CONFIG
{
  "RegData": {
    "MM0":  "0x8182838491929394",
    "MM1":  "0xA1A2A3A4B1B2B3B4"
  },
  "HostFeatures": ["3DNOW"]
}
%endif

movq mm0, [rel data1]
movq mm1, [rel data2]

; Legitimate to implement as a move if the rsqrt or recip instruction does the full calculation
pswapd mm0, [rel data3]
pswapd mm1, [rel data4]

hlt

align 8
data1:
dd 0x41424344
dd 0x51525354

data2:
dd 0x61626364
dd 0x71727374

data3:
dd 0x81828384
dd 0x91929394

data4:
dd 0xA1A2A3A4
dd 0xB1B2B3B4
