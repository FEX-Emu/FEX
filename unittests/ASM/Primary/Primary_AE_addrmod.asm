%ifdef CONFIG
{
  "RegData": {
    "RDI": "0x0000000010000101"
  },
  "MemoryRegions": {
    "0x10000000": "4096"
  }
}
%endif

; Checks SCAS* operations with 0x67 prefix.
; This test ensures that 32-bit address size override works correctly in 64-bit mode

; Set up destination data at address 0x10000100
mov rdi, 0x10000100
mov byte [rdi], 0x41      ; 'A'

; Set initial RDI value with high bits set
; Low 32 bits (EDI) = 0x10000100, high 32 bits = 0x61626364
mov rdi, 0x6162636410000100

; Set AL to match the byte we're scanning for
mov al, 0x41

; This should make the instruction use EDI (32-bit) instead of RDI (64-bit)
; Per x86-64 architecture, writing to 32-bit registers zeros the upper 32 bits
db 0x67
scasb
hlt
