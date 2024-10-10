%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1"
  }
}
%endif

; CPUID function zero
mov rax, 0

cpuid

; CPUID function zero always returns >0 in EAX
cmp eax, 0
mov rax, 0
setnz al

hlt
