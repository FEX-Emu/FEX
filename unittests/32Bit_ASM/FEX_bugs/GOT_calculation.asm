%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x10011"
  },
  "Mode": "32BIT"
}
%endif

mov esp, 0xe0000010

; This is a common pattern in 32-bit PIE code.
; 32-bit GOT calculation needs to do a call+pop to do get the EIP.
; LEA doesn't work because it there is no EIP relative ops like on x86-64.

call target
target:
pop eax

hlt
