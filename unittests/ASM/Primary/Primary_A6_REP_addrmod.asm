%ifdef CONFIG
{
  "RegData": {
    "RSI": "0x0000000010000104",
    "RDI": "0x0000000010000204",
    "RCX": "0x0000000000000000"
  },
  "MemoryRegions": {
    "0x10000000": "4096"
  }
}
%endif

; Checks REP CMPS operation with 0x67 prefix.
; This test ensures that 32-bit address size override works correctly with REP prefix in 64-bit mode

; Source data at 0x10000100
mov rsi, 0x10000100
mov dword [rsi], 0x41424344      ; 'ABCD'

; Destination data at 0x10000200
mov rdi, 0x10000200
mov dword [rdi], 0x41424344      ; same as source

; Set initial RSI/RDI values with high bits set
; Low 32 bits (ESI/EDI) must point to valid memory
mov rsi, 0x5152535410000100
mov rdi, 0x6162636410000200

; Set RCX to number of bytes to compare
mov rcx, 4

; This should make the instruction use ESI/EDI (32-bit) instead of RSI/RDI (64-bit)
; Per x86-64 architecture, writing to 32-bit registers zeros the upper 32 bits
db 0x67
rep cmpsb
hlt
