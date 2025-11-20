%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x10013"
  },
  "Mode": "32BIT"
}
%endif

; Preamble (32Bit_ASM/CMakeLists.txt) sets ES and changes expectation.
; Originally 0x10011, now 0x10013.

mov esp, 0xe0000010

; This is a common pattern in 32-bit PIE code.
; 32-bit GOT calculation needs to do a call+pop to do get the EIP.
; LEA doesn't work because it there is no EIP relative ops like on x86-64.

call target
target:
pop eax

hlt
