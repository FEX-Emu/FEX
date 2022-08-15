%ifdef CONFIG
{
  "RegData": {
      "RAX": "0xFFFFFFFFFFFFFFFF",
      "RBX": "64",
      "RCX": "0x00000000000003FF",
      "RDX": "10",
      "RSI": "0x00000000FFFFFFFF",
      "RDI": "32",
      "RBP": "0x00000000000003FF",
      "RSP": "10"
  },
  "HostFeatures": ["BMI2"]
}
%endif

; Should not alter the source value
mov rax, -1
mov rbx, 64
bzhi rax, rax, rbx

; General operation
mov rcx, -1
mov rdx, 10
bzhi rcx, rcx, rdx

; 32-bit tests

; Should not alter the source value
mov esi, -1
mov edi, 32
bzhi esi, esi, edi

; General operation
mov ebp, -1
mov esp, 10
bzhi ebp, ebp, esp

hlt
