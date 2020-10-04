%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x20",
    "RBX": "0x07",
    "RCX": "0x0F",
    "RDX": "0x17",
    "RSI": "0x1F",
    "RDI": "0x00",
    "RBP": "0x08",
    "RSP": "0x10",
    "R8":  "0x1F",
    "R9":  "0x00",
    "R10": "0x06",
    "R11": "0x0E",
    "R12": "0x16",
    "R13": "0x1E",
    "R14": "0x1D",
    "R15": "0x18"
  }
}
%endif

lea r15, [rel .data]

; We only care about results here
lzcnt eax,  dword [r15 + 4 * 0]
lzcnt ebx,  dword [r15 + 4 * 1]
lzcnt ecx,  dword [r15 + 4 * 2]
lzcnt edx,  dword [r15 + 4 * 3]
lzcnt esi,  dword [r15 + 4 * 4]
lzcnt edi,  dword [r15 + 4 * 5]
lzcnt ebp,  dword [r15 + 4 * 6]
lzcnt esp,  dword [r15 + 4 * 7]
lzcnt r8d,  dword [r15 + 4 * 4]
lzcnt r9d,  dword [r15 + 4 * 9]
lzcnt r10d, dword [r15 + 4 * 10]
lzcnt r11d, dword [r15 + 4 * 11]
lzcnt r12d, dword [r15 + 4 * 12]
lzcnt r13d, dword [r15 + 4 * 13]
lzcnt r14d, dword [r15 + 4 * 14]
lzcnt r15d, dword [r15 + 4 * 15]

hlt

.data:
dd 0x00000000
dd 0x01FFFFFF
dd 0x0001FFFF
dd 0x000001FF
dd 0x00000001
dd 0x80000000
dd 0x00800000
dd 0x00008000
dd 0x00000080
dd 0xFFFFFFFF
dd 0x02000000
dd 0x00020000
dd 0x00000200
dd 0x00000002
dd 0x00000004
