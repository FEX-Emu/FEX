%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x0000000000000000"
  }
}
%endif

mov rsp, 0xe8000000
; RBX accumulates any OF,SF,AF bits (mask 0x890)
; it must end up 0 if OF,SF,AF are being cleared correctly
lea rbp, [rel data]
xor rbx, rbx

%macro run_case 2
  fld qword [rbp]
  fld qword [rbp]

  ; %1 = 0x0   -> OF/SF/AF set to 0
  ; %1 = 0x890 -> OF/SF/AF set to 1
  pushfq
  pop rax
  and rax, ~0x890
  or rax, %1
  push rax
  popfq

  %2

  pushfq
  pop rcx
  and rcx, 0x890 ; isolate OF, SF, AF
  or rbx, rcx

  fstp st0
  fstp st0
%endmacro

run_case 0x0,   fcomi st1
run_case 0x890, fcomi st1

hlt

align 8
data:
    dq 1.5
