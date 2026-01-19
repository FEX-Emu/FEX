%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1",
    "RBP": "0xe0004748",
    "RSP": "0xe0000020"
  }
}
%endif

mov rsp, 0xe0000020
mov rbp, 0x4142434445464748

; Act like an ENTER frame without using ENTER
sub rsp, 2
mov [rsp], bp
mov rbp, rsp
call .target
jmp .end

.target:
mov rax, 1

; operand-size override prefix
; Nasm complains if o16 is used
; `warning: invalid operand size prefix o16, must be o64`
db 0x66
leave

.end:
hlt
