%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000089ABCDEF",
    "RSI": "0x00000000E8000004"
  },
  "MemoryRegions": {
    "0xE8000000": "4096"
  }
}
%endif

; Check LODSD with 0x67 address-size override in 64-bit mode.
; The load and post-increment must use ESI, and the RSI writeback must zero-extend.

mov rsi, 0x12345678E8000000
mov rdx, 0xE8000000
mov dword [rdx], 0x89ABCDEF

db 0x67
lodsd

hlt
