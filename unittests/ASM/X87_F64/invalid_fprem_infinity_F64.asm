%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; Test FPREM with simple operands - reduced precision
finit

; Load simple operands: fprem(1.0, 0.0) should set Invalid Operation
fldz
fld1

; Do FPREM: ST(0) = fprem(ST(0), ST(1)) = fprem(1.0, 0.0)
; fprem(1.0, 0.0) should set Invalid Operation because divisor is zero
fprem

fstsw ax
and rax, 1

hlt
