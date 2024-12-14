%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x000000000f0f0f0f",
    "RBX": "0x000000000f0f0f0f",
    "RCX": "0x000000000f0f0f0f",
    "RDX": "0x00000000ffffffff",
    "R9": "0x000000000f0f0f0f"
  }
}
%endif

; FEX had several bugs in its constprop pass where 32->64 bit truncation behaviour wasn't accounted for leading
; to incorrectly inserting instead.

mov rax, 0x0f0f0f0f0f0f0f0f
mov rbx, 0x0f0f0f0f0f0f0f0f
mov rcx, 0x0f0f0f0f0f0f0f0f
mov rdx, 1
mov r9, 0x0f0f0f0f0f0f0f0f
xor eax, 0
and ebx, ebx
shr ecx, 0
neg edx
shl r9d, 0
hlt
