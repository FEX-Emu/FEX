%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x8000abcd8000fffe",
    "RBX": "0x0000abcc00000001",
    "RCX": "0x0000000000000293"
  }
}
%endif

; FEX had a bug setting carry with 8/16-bit SBB
mov rax, 0x8000abcd80000000
mov rbx, 0x0000abcc00000001

mov rsp, 0xe000_1000

; Start with carry set
stc

jmp .test
.test:

sbb ax, bx

pushfq
pop rcx

hlt
