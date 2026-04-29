%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xcafecafe",
    "RBX": "0x1",
    "RCX": "0x0000000000000011",
    "RDX": "0x1"
  }
}
%endif

; 16-bit RCL effective count is (CL & 0x1F) MOD 17. With CL=17 the count is 0
; so CF and OF must be preserved.

mov rax, 0
mov rbx, 0
mov rdx, 0

mov eax, 1
add eax, 0x7FFFFFFF

mov ax, 0x1234
mov cl, 17

rcl ax, cl

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
