%ifdef CONFIG
{
  "RegData": {
    "RSI": "0x0000000010000101",
    "RDI": "0x0000000010000201"
  },
  "MemoryRegions": {
    "0x10000000": "4096"
  }
}
%endif

; Check CMPS* operations with 0x67 prefix.
; This test ensures that 32-bit address size override works correctly in 64-bit mode

; Set up source data at 0x10000100
mov rsi, 0x10000100
mov byte [rsi], 0x41      ; 'A'

; Set up destination data at 0x10000200
mov rdi, 0x10000200
mov byte [rdi], 0x41      ; same as source

; Set initial RSI/RDI values with high bits set
; Low 32 bits (ESI/EDI) must point to valid memory
mov rsi, 0x5152535410000100
mov rdi, 0x6162636410000200

; This should make the instruction use ESI/EDI (32-bit) instead of RSI/RDI (64-bit)
; Per x86-64 architecture, writing to 32-bit registers zeros the upper 32 bits
db 0x67
cmpsb
hlt
