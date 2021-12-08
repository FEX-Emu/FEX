%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000aaaaafaaa"
  }
}
%endif

%macro intcompare 3
  ; instruction, value1, value2
  mov rcx, %2
  mov rdx, %3
  shl rax, 1
  cmp rcx, rdx

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

; Test integer ops
; RCX and RDX for comparison values
mov rcx, 0
mov rdx, 0

; Test EQ - true
intcompare je, 0, 0

; Test EQ - false
intcompare je, 0, 1

; Test NEQ - true
intcompare jne, 0, 1

; Test NEQ - false
intcompare jne, 0, 0

; Test SGE - true
intcompare jge, 0, 0

; Test SGE - false
intcompare jge, 0, 1

; Test SGE with sign difference - true
intcompare jge, 1, -1

; Test SGE with sign difference - false
intcompare jge, -1, 1

; Test SLT - true
intcompare jl, 0, 1

; Test SLT - false
intcompare jl, 0, 0

; Test SLT with sign difference - true
intcompare jl, -1, 1

; Test SLT with sign difference - false
intcompare jl, 1, -1

; Test SGT - true
intcompare jg, 1, 0

; Test SGT - false
intcompare jg, 0, 0

; Test SGT with sign difference - true
intcompare jg, 1, -1

; Test SGT with sign difference - false
intcompare jg, -1, 1

; Test SLE - true
intcompare jle, 0, 0

; Test SLE - false
intcompare jle, 1, 0

; Test SLE with sign difference - true
intcompare jle, -1, 1

; Test SLE with sign difference - false
intcompare jle, 1, -1

; Test UGE - true
intcompare jae, 0, 0

; Test UGE - false
intcompare jae, 1, 0

; Test UGE with *sign* difference - true
intcompare jae, -1, 1

; Test UGE with *sign* difference - false
intcompare jb, 1, -1

; Test ULT - true
intcompare jb, 0, 1

; Test ULT - false
intcompare jb, 1, 0

; Test ULT with *sign* difference - true
intcompare jb, 1, -1

; Test ULT with *sign* difference - false
intcompare jb, -1, 1

; Test UGT - true
intcompare ja, 1, 0

; Test UGT - false
intcompare ja, 0, 1

; Test UGT with *sign* difference - true
intcompare ja, -1, 1

; Test UGT with *sign* difference - false
intcompare ja, 1, -1

; Test ULE - true
intcompare jbe, 0, 0

; Test ULE - false
intcompare jbe, 1, 0

; Test ULE with *sign* difference - true
intcompare jbe, 1, -1

; Test ULE with *sign* difference - false
intcompare jbe, -1, 1

hlt
