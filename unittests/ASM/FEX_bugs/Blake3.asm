%ifdef CONFIG
{
}
%endif

vblendps ymm3, ymm12, ymm9, 0xcc
vperm2f128 ymm12, ymm1, ymm2, 0x20
vmovups [rel .data_result + 0], ymm12
vunpckhps ymm14, ymm4, ymm5
vblendps ymm4, ymm8, ymm0, 0xcc
vunpckhps ymm15, ymm6, ymm7
vperm2f128 ymm7, ymm3, ymm4, 0x20
vmovups [rel .data_result + 32], ymm7
vshufps ymm5, ymm10, ymm13, 0x4e
vblendps ymm6, ymm5, ymm13, 0xcc
vshufps ymm13, ymm14, ymm15, 0x4e
vblendps ymm10, ymm10, ymm5, 0xcc
vblendps ymm14, ymm14, ymm13, 0xcc
vperm2f128 ymm8, ymm10, ymm14, 0x20
vmovups [rel .data_result + (32 * 2)], ymm8
vblendps ymm15, ymm13, ymm15, 0xcc
vperm2f128 ymm13, ymm6, ymm15, 0x20
vmovups [rel .data_result + (32 * 3)], ymm13
vperm2f128 ymm9, ymm1, ymm2, 0x31
vperm2f128 ymm11, ymm3, ymm4, 0x31
vmovups [rel .data_result + (32 * 4)], ymm9
vperm2f128 ymm14, ymm10, ymm14, 0x31
vperm2f128 ymm15, ymm6, ymm15, 0x31
vmovups [rel .data_result + (32 * 5)], ymm11
vmovups [rel .data_result + (32 * 6)], ymm14
vmovups [rel .data_result + (32 * 7)], ymm15
vmovdqa ymm0, [rel .data_stack + (32 * 0)]
vpaddd  ymm1, ymm0, [rel .data_stack + (32 * 1)]
vmovdqa [rel .data_stack + (32 * 1)], ymm1
vpxor   ymm0, ymm0, [rel .data]
vpxor   ymm2, ymm1, [rel .data + 32]

hlt

align 4096
.data:
dq 0, 0, 0, 0, 0, 0
dq 0, 0, 0, 0, 0, 0

.data_stack:
dq 0, 0, 0, 0
dq 0, 0, 0, 0

.data_result:
dq 0, 0, 0, 0
dq 0, 0, 0, 0
dq 0, 0, 0, 0
dq 0, 0, 0, 0
dq 0, 0, 0, 0
dq 0, 0, 0, 0
dq 0, 0, 0, 0
