%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x000000000cbe0708"
  }
}
%endif

%macro floatcompare 3
  ; instruction, value1, value2
  movsd xmm0, %2
  movsd xmm1, %3

  shl rax, 1
  ucomisd xmm0, xmm1

  ; Conditional branch
  %1 %%true

  %%fallthrough:
    ; False fallthrough path
    mov rbx, 0
    jmp %%combine

  %%true:
    ; True path
    mov rbx, 1

  %%combine:
    ; Combine
    or rax, rbx
%endmacro

; This test specifically tests the Select and compare merging that occurs in OpcodeDispatcher
; The easiest way to test this is to do the comparison op and then SETcc with the flags that we want to ensure is working

; RAX will be our result
mov rax, 0
; RBX will be our temp for setcc
mov rbx, 0

; Float comparisons
; xmm0 and xmm1 will be our comparison values

; FLU (CF == 1), SETNE
; FLEU (CF == 1 || ZF == 1), SETBE
; FU (PF == 1), SETP
; FNU (PF == 0), SETNP
; FGE (CF == 0), SETAE
; FGT (CF == 0 && ZF == 0), SETA

; Test FLU - true
floatcompare jne, [rel .float_0], [rel .float_1]

; Test FLU - false
floatcompare jne, [rel .float_0], [rel .float_1]

; Test FLU (unordered) - true
floatcompare jne, [rel .float_0], [rel .float_qnan]

; Test FLU (unordered) - true
floatcompare jne, [rel .float_qnan], [rel .float_1]

; Test FLEU - true
floatcompare jbe, [rel .float_0], [rel .float_0]

; Test FLEU - false
floatcompare jbe, [rel .float_1], [rel .float_0]

; Test FLEU (unordered) - true
floatcompare jbe, [rel .float_0], [rel .float_qnan]

; Test FLEU (unordered) - true
floatcompare jbe, [rel .float_qnan], [rel .float_1]

; Test FU - true
floatcompare jp, [rel .float_0], [rel .float_qnan]

; Test FU - true
floatcompare jp, [rel .float_qnan], [rel .float_0]

; Test FU - true
floatcompare jp, [rel .float_qnan], [rel .float_qnan]

; Test FU - false
floatcompare jp, [rel .float_1], [rel .float_0]

; Test FU - false
floatcompare jp, [rel .float_0], [rel .float_1]

; Test FU - false
floatcompare jp, [rel .float_0], [rel .float_0]

; Test FNU - false
floatcompare jnp, [rel .float_0], [rel .float_qnan]

; Test FNU - false
floatcompare jnp, [rel .float_qnan], [rel .float_0]

; Test FNU - false
floatcompare jnp, [rel .float_qnan], [rel .float_qnan]

; Test FNU - true
floatcompare jnp, [rel .float_1], [rel .float_0]

; Test FNU - true
floatcompare jnp, [rel .float_0], [rel .float_1]

; Test FNU - true
floatcompare jnp, [rel .float_0], [rel .float_0]

; Test FGE - true
floatcompare ja, [rel .float_0], [rel .float_0]

; Test FGE - false
floatcompare ja, [rel .float_0], [rel .float_1]

; Test FGE (unordered) - false
floatcompare ja, [rel .float_0], [rel .float_qnan]

; Test FGE (unordered) - false
floatcompare ja, [rel .float_qnan], [rel .float_1]

; Test FGT - true
floatcompare ja, [rel .float_1], [rel .float_0]

; Test FGT - false
floatcompare ja, [rel .float_0], [rel .float_1]

; Test FGT (unordered) - false
floatcompare ja, [rel .float_0], [rel .float_qnan]

; Test FGT (unordered) - false
floatcompare ja, [rel .float_qnan], [rel .float_1]

hlt

align 8
.float_1:
dq 1.0
.float_0:
dq 0.0
.float_qnan:
dq 0x7ff8000000000000
