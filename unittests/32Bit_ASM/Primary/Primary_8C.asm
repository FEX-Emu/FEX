%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFF0033",
    "RBX": "0xFFFF0033",
    "RCX": "0xFFFF0033"
  },
  "Mode": "32BIT"
}
%endif

mov eax, 0x33
mov gs, ax
mov fs, ax
mov es, ax

mov eax, 0xFFFFFFFF
mov ebp, 0xe0000000

; Store 32bits of data
mov dword [ebp + 0], eax
mov dword [ebp + 4], eax
mov dword [ebp + 8], eax

; Ensure that the segment store only writes 16-bits
mov word [ebp + 0], gs
mov word [ebp + 4], fs
mov word [ebp + 8], es

mov eax, 0
mov ebx, 0
mov ecx, 0
mov eax, dword [ebp + 0]
mov ebx, dword [ebp + 4]
mov ecx, dword [ebp + 8]

hlt
