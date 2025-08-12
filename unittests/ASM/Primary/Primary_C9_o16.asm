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
o16 leave

.end:
hlt
