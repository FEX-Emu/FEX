%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x244ed7",
    "RBX": "0x240202"
  }
}
%endif

mov rsp, 0xe8000020

; Try to set VIP, VIF VM, IOPL, IF and RF in EFLAGS
mov rax, 0x3f7cd5
push rax
popfq

pushfq
mov rax, qword [rsp]


; Pre-set AC and ID via a 64-bit POPFQ.
mov rbx, 0x240000
push rbx
popfq

; Try to clear AC and ID
mov rbx, 0
push rbx

o16 popfq

; Read flags
pushfq
mov rbx, qword [rsp]

hlt
