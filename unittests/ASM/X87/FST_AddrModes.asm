%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x3f800000",
    "RBX": "0x3f800000",
    "RCX": "0x3f800000",
    "R8": "0x3f800000",
    "R9": "0x3f800000",
    "R10": "0x3f800000"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

; Setup
fld1
lea rdx, [rel base]
mov rsi, 0x64

; Test fst
fst dword [rdx]
fst dword [rdx + 0xa]
fst dword [rdx + rsi]
fst dword [rdx + rsi * 4]
fst dword [rdx + rsi + 0xa]
fst dword [rdx + rsi * 4 + 0xa]

; Result check
mov eax, dword [rdx]
mov ebx, dword [rdx + 0xa]
mov ecx, dword [rdx + rsi]
mov r8d, dword [rdx + rsi * 4]
mov r9d, dword [rdx + rsi + 0xa]
mov r10d, dword [rdx + rsi * 4 + 0xa]

hlt

align 4096
base:
times 4096 db 0
