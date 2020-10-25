%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x6",
    "RCX": "0x5",
    "RDX": "0x4",
    "RSP": "0xE0000020",
    "RBX": "0x3",
    "RBP": "0x2",
    "RSI": "0x1",
    "RDI": "0x0"
  },
  "Mode": "32BIT"
}
%endif

mov esp, 0xe0000020

push dword 0x6
push dword 0x5
push dword 0x4
push dword 0x3
push dword 0x41424344 ; Skipped
push dword 0x2
push dword 0x1
push dword 0x0

popad

hlt
