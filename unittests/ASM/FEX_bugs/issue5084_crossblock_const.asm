%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0"
  }
}
%endif

; Regression test for issue 5084
; This test was mostly reverse engineered from the IR in 5084.
; Failed with '-n 500 -m' with the error message:
; %51: Arg[0] references invalid %24

mov rax, 0
mov rbx, 0xe0000000
mov rcx, 1

test rcx, rcx
jnz .late_target

.fallthrough:
fld1
fstp tword [rbx + 0x1234]
mov rax, 0
hlt

.late_target:
fld1
fstp tword [rbx + 0x1234]
mov rax, 0
hlt
