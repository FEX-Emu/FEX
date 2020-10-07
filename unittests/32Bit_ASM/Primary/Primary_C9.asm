%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1",
    "RBP": "0x41424344"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov esp, 0xe0000020
mov ebp, 0x41424344

; Act like an ENTER frame without using ENTER
push ebp
mov ebp, esp
call .target
jmp .end

.target:
mov eax, 1
leave

.end:
hlt
