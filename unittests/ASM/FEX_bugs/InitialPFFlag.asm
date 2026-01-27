%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x202"
  }
}
%endif

mov rsp, 0xe0000010

pushfq
pop rax

; Mask out only the flags we care about (ignore undefined bits)
; Keep: CF(0), PF(2), AF(4), ZF(6), SF(7), IF(9), DF(10), OF(11), reserved(1)
and rax, 0xED7

hlt
