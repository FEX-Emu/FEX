%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x3f800000",
    "RBX": "0x3f800000",
    "RCX": "0x3f800000",
    "RBP": "0x3f800000",
    "RDI": "0x3f800000",
    "RSP": "0x3f800000"
  },
  "MemoryRegions": {
    "0xf0000000": "4096"
  },
  "Mode": "32BIT"
}
%endif

section .bss
base resb 4096

section .text

; Setup
fld1
lea edx, [rel base]
mov esi, 0x64

; Test fst
fst dword [edx]
fst dword [edx + 0xa]
fst dword [edx + esi]
fst dword [edx + esi * 4]
fst dword [edx + esi + 0xa]
fst dword [edx + esi * 4 + 0xa]

; Result check
mov eax, dword [edx]
mov ebx, dword [edx + 0xa]
mov ecx, dword [edx + esi]
mov ebp, dword [edx + esi * 4]
mov edi, dword [edx + esi + 0xa]
mov esp, dword [edx + esi * 4 + 0xa]

hlt
