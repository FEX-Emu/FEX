%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464748",
    "RBX": "0x5152535455560006",
    "RCX": "0x6162636465660006",
    "RDX": "0x5152535455560003",
    "RDI": "0x6162636465660003",
    "RSI": "0x5152535455560001",
    "RSP": "0x6162636465660001"
  }
}
%endif

; FEX-Emu had a bug where the secondary two-byte table decoding failed to decode the 0xBX nibble correctly.
; Depending on the order of the prefix FEX chose an incorrect instruction or invalid instruction depending.
; Found in Order of the Sinking Star (SteamAppID 499170).

mov rax, 0x4142434445464748
mov rbx, 0x5152535455565758
mov rcx, 0x6162636465666768

mov rdx, 0x5152535455565758
mov rdi, 0x6162636465666768

mov rsi, 0x5152535455565758
mov rsp, 0x6162636465666768

db 0x66, 0xf3, 0x0f, 0xb8, 0xd8 ; popcnt bx, ax
db 0xf3, 0x66, 0x0f, 0xb8, 0xc8 ; popcnt cx, ax

db 0x66, 0xf3, 0x0f, 0xbc, 0xd0 ; tzcnt dx, ax
db 0xf3, 0x66, 0x0f, 0xbc, 0xf8 ; tzcnt di, ax

db 0x66, 0xf3, 0x0f, 0xbd, 0xf0 ; lzcnt si, ax
db 0xf3, 0x66, 0x0f, 0xbd, 0xe0 ; lzcnt sp, ax

hlt
