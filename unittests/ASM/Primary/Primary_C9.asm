%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1",
    "RBP": "0x4142434445464748"
  }
}
%endif

mov rsp, 0xe0000020
mov rbp, 0x4142434445464748

; Act like an ENTER frame without using ENTER
push rbp
mov rbp, rsp
call .target
jmp .end

.target:
mov rax, 1
leave

.end:
hlt
