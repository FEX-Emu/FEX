%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000cafecafe"
  }
}
%endif

; IsNan() couldn't detect negative NaNs (sign bit set in exponent field).
; This caused __builtin_isunordered() to return wrong values.

mov rsp, 0xe000_1000

; Test 1: __builtin_isunordered(1.0, 2.0) should return 0
; Pattern: fucomip + setp + test for 0
fld1
lea rdx, [rel two]
fld tword [rdx]
fucomip st1
setp al
movzx eax, al
test eax, eax
jnz test_failed         ; If not 0, test failed (should be ordered)

; Test 2: __builtin_isunordered(1.0, NaN) should return 1  
fld1
lea rdx, [rel qnan]
fld tword [rdx]
fucomip st1
setp al
movzx eax, al
test eax, eax
jz test_failed          ; If 0, test failed (should be unordered)

; Test 3: __builtin_isunordered(NaN, 1.0) should return 1
lea rdx, [rel qnan]
fld tword [rdx]
fld1
fucomip st1
setp al
movzx eax, al
test eax, eax
jz test_failed          ; If 0, test failed (should be unordered)

; Test 4: __builtin_isunordered(2.0, 2.0) should return 0 (equal case)
lea rdx, [rel two]
fld tword [rdx]
lea rdx, [rel two]  
fld tword [rdx]
fucomip st1
setp al
movzx eax, al
test eax, eax
jnz test_failed         ; If not 0, test failed (should be ordered)

; All tests passed
mov rax, 0xcafecafe
hlt

test_failed:
; Test failed 
mov rax, 0xdeadbeef
hlt

align 8
two:
  dt 2.0

align 8  
qnan:
  dq 0xC000000000000000  ; Quiet NaN with only quiet bit set (no bottom 62 bits) - this breaks IsNan
  dw 0x7FFF              ; Standard NaN exponent (0x7FFF)