%ifdef CONFIG
{
  "RegData": {
      "RBX": "0",
      "RDX": "0xFF",
      "RSI": "0",
      "R8" : "0xDEADBEEFDEADBEEF",
      "R9" : "0xDEADBEEF",
      "R14": "0x7F",
      "R15": "0x838"
  },
  "HostFeatures": ["BMI1"]
}
%endif

; General extraction
mov r14, 0x7FFFFFFFFFFFFFFF
mov r15, 0x838              ; Start at bit 56 and extract 8 bits
bextr r14, r14, r15         ; This results in 0x7F being placed into RAX

; Extraction with 0 bits should clear the destination
mov rbx, -1
mov rcx, 0
bextr rbx, rbx, rcx

; Extraction with 'SrcSize' bits should get the unchanged register
mov r8, 0xDEADBEEFDEADBEEF
mov r9, 16384              ; Start at 0 extract 64 bits
bextr r8, r8, r9           ; r8 should stay the same

; Same tests as above but with 32-bit registers

; General extraction
mov rdx, 0x7FFFFFFFFFFFFFFF
mov rsi, 0x818              ; Start at bit 24 and extract 8 bits
bextr edx, edx, esi         ; This results in 0xFF being placed into EDX

; Extraction with 0 bits should clear RSI to 0
mov rsi, -1
mov rdi, 0
bextr esi, esi, edi

; Extraction with 'SrcSize' bits should get the unchanged register
mov r9, 0xDEADBEEFDEADBEEF
mov r10, 8192               ; Start at 0 extract 32 bits
bextr r9d, r9d, r10d        ; r9 should become 0xDEADBEEF (and r9d stays the same)

hlt
