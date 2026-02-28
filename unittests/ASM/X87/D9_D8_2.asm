%ifdef CONFIG
{
  "RegData": {
    "MM5": ["0x8000000000000000", "0x4001"],
    "MM6": ["0x8000000000000000", "0x3FFF"],
    "MM7": ["0xC000000000000000", "0x4000"]
  }
}
%endif

; Tests undocumented FSTPNCE with ST(2) target.
; Verifies the register index extraction (OP & 7) works for non-ST(1) targets.
;
; Related: issue #5296

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

lea rdx, [rel data2]
fld tword [rdx + 8 * 0]

lea rdx, [rel data3]
fld tword [rdx + 8 * 0]

; FSTPNCE ST(2) = db 0xD9, 0xDA
; Stack before: ST(0)=1.0, ST(1)=3.0, ST(2)=2.0
; Store ST(0) (1.0) into ST(2), then pop.
; After pop: ST(0)=3.0, ST(1)=1.0
db 0xD9, 0xDA

lea rdx, [rel data4]
fld tword [rdx + 8 * 0]

; Final stack: ST(0)=4.0, ST(1)=3.0, ST(2)=1.0
; 4.0: exponent=0x4001, mantissa=0x8000000000000000
; 3.0: exponent=0x4000, mantissa=0xC000000000000000
; 1.0: exponent=0x3FFF, mantissa=0x8000000000000000

hlt

align 8
data:
  dt 2.0
  dq 0
data2:
  dt 3.0
  dq 0
data3:
  dt 1.0
  dq 0
data4:
  dt 4.0
  dq 0
