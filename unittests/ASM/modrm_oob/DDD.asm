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

; DDD
r3 pi2fw, 8, mm0
r3 pi2fd, 8, mm0
r3 pf2iw, 8, mm0
r3 pf2id, 8, mm0
r3 pfrcpv, 8, mm0
r3 pfrsqrtv, 8, mm0
r3 pfnacc, 8, mm0
r3 pfpnacc, 8, mm0
r3 pfcmpge, 8, mm0
r3 pfmin, 8, mm0
r3 pfrcp, 8, mm0
r3 pfrsqrt, 8, mm0
r3 pfsub, 8, mm0
r3 pfadd, 8, mm0
r3 pfcmpgt, 8, mm0
r3 pfmax, 8, mm0
r3 pfrcpit1, 8, mm0
r3 pfrsqit1, 8, mm0
r3 pfsubr, 8, mm0
r3 pfacc, 8, mm0
r3 pfcmpeq, 8, mm0
r3 pfmul, 8, mm0
r3 pfrcpit2, 8, mm0
; Nasm doesn't understand this instruction.
; r3 pmulhrw, 8, mm0
r3 pswapd, 8, mm0
r3 pavgusb, 8, mm0

; Done
mov rax, 1
hlt
