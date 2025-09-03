%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x000000005c3bc5b0",
    "RBX": "0x000000001dd5b1e5",
    "RCX": "0x0000000015d1c92d"
  },
  "HostFeatures": ["SSE4.2"]
}
%endif

mov rax, 0x41424344454647
mov rbx, 0x51525354555657
mov rcx, 0x61626364656667

mov rdx, 0x71727374757677

; crc32 rax, rbx
db 0x66 ; Override, Should be ignored
db 0xf2 ; Prefix
db 0x48 ; REX.W
db 0x0f, 0x38, 0xf1, 0xc3

; crc32 rbx, rcx
db 0xf2 ; Prefix
db 0x66 ; Override, Should be ignored
db 0x48 ; REX.W
db 0x0f, 0x38, 0xf1, 0xd9

; crc32 rcx, rdx
db 0x66 ; Override, Should be ignored
db 0xf2 ; Prefix
db 0x66 ; Override, Should be ignored
db 0x48 ; REX.W
db 0x0f, 0x38, 0xf1, 0xca

hlt
