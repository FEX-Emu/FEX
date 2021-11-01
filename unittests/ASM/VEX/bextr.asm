%ifdef CONFIG
{
  "RegData": {
      "RAX": "0x7F",
      "RBX": "0",
      "RDX": "0xFF",
      "RSI": "0"
  }
}
%endif

; General extraction
mov rax, 0x7FFFFFFFFFFFFFFF
mov rbx, 0x838              ; Start at bit 56 and extract 8 bits
bextr rax, rax, rbx         ; This results in 0x7F being placed into RAX

; Extraction with 0 bits should clear the destination
mov rbx, -1
mov rcx, 0
bextr rbx, rbx, rcx

; Same tests as above but with 32-bit registers

; General extraction
mov rdx, 0x7FFFFFFFFFFFFFFF
mov rsi, 0x818              ; Start at bit 24 and extract 8 bits
bextr edx, edx, esi         ; This results in 0xFF being placed into EDX

; Extraction with 0 bits should clear RSI to 0
mov rsi, -1
mov rdi, 0
bextr esi, esi, edi

hlt