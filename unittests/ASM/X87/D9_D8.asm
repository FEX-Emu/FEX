%ifdef CONFIG
{
  "RegData": {
    "MM6": ["0x8000000000000000", "0x4001"],
    "MM7": ["0x8000000000000000", "0x3FFF"]
  }
}
%endif

; Tests undocumented FSTPNCE (D9 D8+i) instruction.
; FSTPNCE behaves as FSTP ST(i) - stores ST(0) to ST(i) and pops.
; Raw byte encoding required since assemblers don't recognize this mnemonic.
;
; Related: issue #5296

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

lea rdx, [rel data2]
fld tword [rdx + 8 * 0]

; FSTPNCE ST(1) = db 0xD9, 0xD9
; This stores ST(0) (1.0) into ST(1), then pops.
; After pop, old ST(1) (which now holds 1.0) becomes ST(0).
db 0xD9, 0xD9

lea rdx, [rel data3]
fld tword [rdx + 8 * 0]

; Expected stack: ST(0)=4.0 (MM7), ST(1)=1.0... wait.
; Let me trace:
;   fld 2.0 -> ST(0)=2.0
;   fld 1.0 -> ST(0)=1.0, ST(1)=2.0
;   FSTPNCE ST(1) -> ST(1) = ST(0) = 1.0, then pop -> ST(0)=1.0
;   fld 4.0 -> ST(0)=4.0, ST(1)=1.0
; Result: MM7=[4.0], MM6=[1.0]
; 4.0 in x87: exponent=0x4001, mantissa=0x8000000000000000
; 1.0 in x87: exponent=0x3FFF, mantissa=0x8000000000000000

hlt

align 8
data:
  dt 2.0
  dq 0
data2:
  dt 1.0
  dq 0
data3:
  dt 4.0
  dq 0
