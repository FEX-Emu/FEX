%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "MemoryRegions": {
    "0x100000000": "4096",
    "0x100002000": "4096"
  }
}
%endif

mov r15, 0x100001000
mov r14, 0x100002000
mov rax, 0

%include "modrm_oob_macros.mac"

; VEX group 15
w2 vstmxcsr, 4
w2 vldmxcsr, 4

; VEX group 17
r3 blsr, 4, eax
r3 blsr, 8, rax
r3 blsmsk, 4, eax
r3 blsmsk, 8, rax
r3 blsi, 4, eax
r3 blsi, 8, rax

; Done
mov rax, 1
hlt
