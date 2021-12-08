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
  cmp rcx, rdx
  %1 bl
  shl rax, 1
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
intcompare sete, 0, 0

; Test EQ - false
intcompare sete, 0, 1

; Test NEQ - true
intcompare setne, 0, 1

; Test NEQ - false
intcompare setne, 0, 0

; Test SGE - true
intcompare setge, 0, 0

; Test SGE - false
intcompare setge, 0, 1

; Test SGE with sign difference - true
intcompare setge, 1, -1

; Test SGE with sign difference - false
intcompare setge, -1, 1

; Test SLT - true
intcompare setl, 0, 1

; Test SLT - false
intcompare setl, 0, 0

; Test SLT with sign difference - true
intcompare setl, -1, 1

; Test SLT with sign difference - false
intcompare setl, 1, -1

; Test SGT - true
intcompare setg, 1, 0

; Test SGT - false
intcompare setg, 0, 0

; Test SGT with sign difference - true
intcompare setg, 1, -1

; Test SGT with sign difference - false
intcompare setg, -1, 1

; Test SLE - true
intcompare setle, 0, 0

; Test SLE - false
intcompare setle, 1, 0

; Test SLE with sign difference - true
intcompare setle, -1, 1

; Test SLE with sign difference - false
intcompare setle, 1, -1

; Test UGE - true
intcompare setae, 0, 0

; Test UGE - false
intcompare setae, 1, 0

; Test UGE with *sign* difference - true
intcompare setae, -1, 1

; Test UGE with *sign* difference - false
intcompare setb, 1, -1

; Test ULT - true
intcompare setb, 0, 1

; Test ULT - false
intcompare setb, 1, 0

; Test ULT with *sign* difference - true
intcompare setb, 1, -1

; Test ULT with *sign* difference - false
intcompare setb, -1, 1

; Test UGT - true
intcompare seta, 1, 0

; Test UGT - false
intcompare seta, 0, 1

; Test UGT with *sign* difference - true
intcompare seta, -1, 1

; Test UGT with *sign* difference - false
intcompare seta, 1, -1

; Test ULE - true
intcompare setbe, 0, 0

; Test ULE - false
intcompare setbe, 1, 0

; Test ULE with *sign* difference - true
intcompare setbe, 1, -1

; Test ULE with *sign* difference - false
intcompare setbe, -1, 1

hlt
