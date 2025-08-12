%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x41424344fbca7654",
    "RBX": "0x00000000fbca7654",
    "RCX": "0x61626364fbca7654"
  }
}
%endif

; FEX-Emu had a but where it was failing to follow zero-extend semantics on
; the destination register when cmpxchg was a success as a 32-bit operation.
; A simple test that does a 32-bit compare exchange with success as 32-bit.
mov rax, [rel .data + (8 * 0)]
mov rbx, [rel .data + (8 * 0)]
mov rcx, [rel .data + (8 * 1)]

cmpxchg ebx, ecx
hlt

.data:
dq 0x41424344_fbca7654
dq 0x61626364_fbca7654
