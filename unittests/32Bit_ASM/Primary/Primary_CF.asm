%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1",
    "RSP": "0xe0000010"
  },
  "Mode": "32BIT"
}
%endif

mov esp, 0xe0000010

lea ebx, dword [rel .end]

mov eax, 0x202
push eax ; RFLAGS
mov eax, 0x33
push eax ; CS
push ebx ; RIP

mov eax, -1
iretd

; Super fail
mov eax, 2
hlt

.end_fail:
mov eax, 0
hlt

.end:
mov eax, 1

hlt
