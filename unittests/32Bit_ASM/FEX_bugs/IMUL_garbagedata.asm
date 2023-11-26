%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x000000007dbf2800",
    "RDX": "0x0000000000000000",
    "RBX": "0x000000000000004f",
    "RCX": "0x000000000000004f",
    "RBP": "0x0000000000009e4f",
    "RSI": "0x0000000000009e4f",
    "RSP": "0x000000000000004f"
  },
  "Mode": "32BIT"
}
%endif

; FEX had a bug where smaller than 64-bit imul could leave garbage data in the upper 32-bits of the 32-bit result.
; This would cause subsequent instructions after the imul to receive garbage bits.
; In particular this would feed in to address calculation in DXVK with "Dungeon Defenders" doing address calculation.
; The address calculation did something similar to:
;   xor edx, edx
;   mov eax, 0x7dbf2800
;   imul ebx, ebx, 0xaaaaaaab
;   div ebx
; Divide expected 0x4f but received 0xffffffb1'0000'004f

; Dividend
xor edx, edx
mov eax, 0x7dbf2800

; Multiply starting value
mov ebx, 0xED

jmp .test

.test:

; imul 1-src
mov edi, 0xaaaaaaab
imul di, bx
mov esp, 0xaaaaaaab
imul esp, ebx

; imul 2-src 8-bit check
imul bp, bx, 0xab
imul esi, ebx, 0xab

; imul 2-src 16-bit check
imul cx, bx, 0xaaab
imul ebx, ebx, 0xaaaaaaab

hlt
