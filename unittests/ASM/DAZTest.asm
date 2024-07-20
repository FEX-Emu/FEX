%ifdef CONFIG
{
  "HostFeatures": ["AFP"],
  "RegData": {
    "XMM0": ["0x0108000040e00000", "0xd1d2d3d4d5d6d7d8", "0", "0"],
    "XMM1": ["0x00cfffff40e00000", "0xd1d2d3d4d5d6d7d8", "0", "0"]
  }
}
%endif

vmovaps ymm1, [rel .data_three]
vmovaps ymm2, [rel .data_four]

; Do an add without DAZ
vaddps xmm0, xmm1, xmm2

; Set DAZ
stmxcsr [rel .data_mxcsr]
or dword [rel .data_mxcsr], (1 << 6)
ldmxcsr [rel .data_mxcsr]

; Do an add with DAZ
vaddps xmm1, xmm1, xmm2

hlt
align 32

.data_three:
dd 3.0, 0x00cfffff
dq 0xa1a2a3a4a5a6a7a8, 0xb1b2b3b4b5b6b7b8, 0xc1c2c3c4c5c6c7c8

.data_four:
dd 4.0, 0x00400000
dq 0xd1d2d3d4d5d6d7d8, 0xe1e2e3e4e5e6e7e8, 0xf1f2f3f4f5f6f7f8

.data_mxcsr:
dd 0
