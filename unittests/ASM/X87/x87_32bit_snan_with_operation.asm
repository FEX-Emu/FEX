%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x7FC00001"
  }
}
%endif

mov rsp, 0xe0000040

; Test x87 signaling NaN silencing with an operation in between
; This forces the use of x87 stack operations instead of direct load/store
; A 32-bit sNaN has all exponent bits set and bit 22 clear
; After an operation and store, bit 22 should be set, making it a qNaN
; Value: 0x7F800001 (sNaN) should become 0x7FC00001 (qNaN with bit 22 set)

finit
lea rdx, [rel data]
fld dword [rdx]           ; Load 32-bit sNaN onto x87 stack
fld dword [rdx]           ; Load it again - now we have st0=sNaN, st1=sNaN
fadd                      ; Add them - st0 = sNaN + sNaN = sNaN (NaN propagates)
fstp dword [rdx + 16]     ; Store result - should silence to qNaN

; Load the result and check bit 22 is set
mov eax, [rdx + 16]

hlt

align 4096
data:
  dd 0x7F800001             ; 32-bit signaling NaN (exp=0xFF, frac=1, bit 22 clear)
  dd 0
  dd 0
  dd 0                      ; space for result
