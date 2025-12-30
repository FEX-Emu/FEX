%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000055565758",
    "RCX": "0x5152535455565758",
    "R8": "0"
  }
}
%endif

mov rax, 0x4142434445464748
mov rcx, 0x4142434445464748
mov r8, 0
movups xmm0, [rel .data]
jmp .test
.test:

; pextrd eax, xmm0, 0
; Real encoding: 0x66, 0x0f, 0x3a, 0x16, 0xc0, 0x00
; Add a NOP REX encoding. Would convert `eax` to `rax` if decoded incorrectly.
db 0x4f, 0x66, 0x0f, 0x3a, 0x16, 0xc0, 0x00
; pextrq rcx, xmm0, 0
; Real encoding: 0x66, 0x48, 0x0f, 0x3a, 0x16, 0xc1, 0x00
; Add a NOP REX encoding, should do nothing. Might convert rcx to ecx if only first REX decoded.
db 0x47, 0x66, 0x48, 0x0f, 0x3a, 0x16, 0xc1, 0x00
hlt

align 16
.data:
dq 0x5152535455565758, 0x6162636465666768
