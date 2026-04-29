%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xcafecafe",
    "RBX": "0x1",
    "RCX": "0x0000000000000009",
    "RDX": "0x1"
  }
}
%endif

; 8-bit RCR effective count is (CL & 0x1F) MOD 9. With CL=9 the count is 0
; so CF and OF must be preserved.

mov rax, 0
mov rbx, 0
mov rdx, 0

mov eax, 1
add eax, 0x7FFFFFFF

mov al, 0xC0
mov cl, 9
rcr al, cl

jo .of_ok
mov rax, 0xdead0001
hlt
.of_ok:
mov rbx, 1

jnc .cf_ok
mov rax, 0xdead0002
hlt
.cf_ok:
mov rdx, 1

mov rax, 0xcafecafe
hlt
