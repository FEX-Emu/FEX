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

; Secondary ModRM

; clzero is a bit special
lea rax, [r15 - 64]
clzero rax

lea rax, [r14]
clzero rax

; Done
mov rax, 1
hlt
