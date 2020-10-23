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

mov eax, 0
mov ecx, 1
mov edx, 2
mov ebx, 3
mov ebp, 4
mov esi, 5
mov edi, 6

pushad

; Invert the order

mov eax, [esp + 4 * 0]
mov ecx, [esp + 4 * 1]
mov edx, [esp + 4 * 2]
; sp here
mov ebx, [esp + 4 * 4]
mov ebp, [esp + 4 * 5]
mov esi, [esp + 4 * 6]
mov edi, [esp + 4 * 7]

; Load sp last
mov esp, [esp + 4 * 3]


hlt
