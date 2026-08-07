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


; Since AC is set, alignment checking is enabled -> realign RSP
mov rsp, 0xe0000020
pushfq
pop rbx

; Clear AC, as under native FEX host execution
; the test will crash later in the TestHarnessRunner
push 0
popfq

hlt
