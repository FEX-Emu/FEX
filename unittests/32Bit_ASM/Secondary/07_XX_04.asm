%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000080050033",
    "RBX": "0x0000000041420033",
    "RCX": "0x0000000041420033",
    "RDX": "0x0000000041420033",
    "RDI": "0x0000000080050033",
    "RSP": "0x0000000080050033",
    "RBP": "0x0000000041420033"
  },
  "Mode": "32BIT"
}
%endif

mov eax, 0x41424344
mov ebx, 0x41424344
mov ecx, 0x41424344
mov edx, 0x41424344
mov esi, 0xe000_0000
mov [esi], edx

mov edi, 0x41424344
mov esp, 0x41424344
mov ebp, 0x41424344

smsw eax

; Newer nasm doesn't encode 16-bit smsw with 16-bit register argument anymore.
db 0x66
smsw ebx

smsw [esi]
mov ecx, [esi]

db 0x66
smsw edx
repe smsw edi
repne smsw esp

db 0x66
smsw ebp

hlt
