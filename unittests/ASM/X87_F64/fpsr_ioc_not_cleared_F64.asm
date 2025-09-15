%ifdef CONFIG
{
  "RegData": {
    "RAX": "0"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; Test FPSR IOC bit not being cleared properly
; 1. SSE operation sets FPSR IOC bit
; 2. Valid x87 operation should have clean status but IOC contaminates it
; 3. x87 status check shows IE=1 incorrectly

fninit

; sets FPSR IOC bit
mov eax, 0xC0800000
movd xmm0, eax
sqrtss xmm1, xmm0       ; sqrt(-4.0) sets FPSR IOC on the arm side through fsqrt

; good op
fld1
fld1
fadd

fstsw ax
and eax, 1 ; unless we get contamination IE flag is 0.

hlt
