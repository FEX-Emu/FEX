%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xcafecafe"
  },
  "Mode": "32BIT"
}
%endif

; IsNan() couldn't detect negative NaNs (sign bit set in exponent field).
; This caused __builtin_isunordered() to return wrong values.

mov esp, 0xe000_1000

; Test 1: __builtin_isunordered(1.0, 2.0) should return 0
; Pattern: fucomip + setp + test for 0
fld1
lea edx, [two]
fld tword [edx]
fucomip st1
setp al
movzx eax, al
test eax, eax
jnz test_failed         ; If not 0, test failed (should be ordered)

; Test 2: __builtin_isunordered(1.0, NaN) should return 1  
fld1
lea edx, [qnan]
fld tword [edx]
fucomip st1
setp al
movzx eax, al
test eax, eax
jz test_failed          ; If 0, test failed (should be unordered)

; Test 3: __builtin_isunordered(NaN, 1.0) should return 1
lea edx, [qnan]
fld tword [edx]
fld1
fucomip st1
setp al
movzx eax, al
test eax, eax
jz test_failed          ; If 0, test failed (should be unordered)

; Test 4: __builtin_isunordered(2.0, 2.0) should return 0 (equal case)
lea edx, [two]
fld tword [edx]
lea edx, [two]
fld tword [edx]
fucomip st1
setp al
movzx eax, al
test eax, eax
jnz test_failed         ; If not 0, test failed (should be ordered)

; All tests passed
mov eax, 0xcafecafe
hlt

test_failed:
; Test failed 
mov eax, 0xdeadbeef
hlt

align 8
two:
  dt 2.0

align 8  
qnan:
  dq 0xC000000000000000  ; Quiet NaN with only quiet bit set (no bottom 62 bits) - this breaks IsNan
  dw 0x7FFF              ; Standard NaN exponent (0x7FFF)