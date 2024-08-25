%ifdef CONFIG
{
}
%endif

; FEX-Emu had a bug in the vcvtdq2ps and vcvtdq2pd instruction where it was incorrectly generating a 256-bit IR operation.
; Due to a quirk of the IR operation handling, this instruction was actually handled "correctly" as a 128-bit operation.
; The problem occured once there was enough live registers to cause spilling, and the register spiller tries to spill the full result.
; The full result in this case was described as a 256-bit operation when it was supposed to be only a 128-bit operation.
; This was found in `Aperture Desk Job` in `libphonon.so` in function `own_ipps_sLn_L9LAynn`.
jmp .test

.test:
vmovups ymm4,  [rel data_7ffde364df00]
vmovups ymm5,  [rel data_7ffde364df00]
vmovups ymm6,  [rel data_7ffde364df00]
vmovups ymm7,  [rel data_7ffde364df00]
vmovups ymm8,  [rel data_7ffde364df00]
vmovups ymm9,  [rel data_7ffde364df00]
vmovups ymm10,  [rel data_7ffde364df00]
vmovups ymm11,  [rel data_7ffde364df00]

vpsubd  ymm12, ymm0, ymm4
vpsubd  ymm13, ymm1, ymm4
vcvtdq2ps ymm2, ymm2
vcvtdq2ps ymm14, ymm14
vmovmskps ecx, ymm15
hlt

align 32
data_7ffde364df00:
dq 0, 0, 0, 0
