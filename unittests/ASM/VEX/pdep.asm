%ifdef CONFIG
{
  "RegData": {
      "RAX": "0x00012567",
      "RBX": "0xFF00FFF0",
      "RCX": "0x12005670",
      "RDX": "0x0801256708012567",
      "RSI": "0xFF00FF00FF00FF00",
      "RDI": "0x0800010025006700"
  },
  "HostFeatures": ["BMI2"]
}
%endif

; 32-bit
mov eax, 0x00012567
mov ebx, 0xFF00FFF0
pdep ecx, eax, ebx

; 64-bit
mov rdx, 0x0801256708012567
mov rsi, 0xFF00FF00FF00FF00
pdep rdi, rdx, rsi

hlt
