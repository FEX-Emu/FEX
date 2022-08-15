%ifdef CONFIG
{
  "RegData": {
      "RAX": "0xFFFFFFFFFFFFFFFE",
      "RBX": "4",
      "RCX": "4",
      "RDX": "0xFFFFFFFE",
      "RSI": "1",
      "RDI": "1"
  },
  "HostFeatures": ["BMI2"]
}
%endif

; Test low
mov rbx, 2
mov rdx, 2
mulx rax, rbx, rbx

; Test high
mov rcx, -1
mov rdx, -1
mulx rax, rdi, rcx

; 32-bit

; Test low
mov ecx, 2
mov edx, 2
mulx edx, ecx, ecx

; Test high
mov esi, -1
mov edx, -1
mulx edx, esi, esi 

hlt
