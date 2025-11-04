; NaN Testing Macros for Assembly Tests
; Implements NaN triple testing system:
; - Bit 2: 1 if value is NaN
; - Bit 1: 1 if quiet NaN
; - Bit 0: 1 if signaling NaN
;
; Triple values:
; 0b000 (0): Not a NaN
; 0b101 (5): Signaling NaN
; 0b110 (6): Quiet NaN
;
; ASSUMPTION: All input pointers (rdx) are valid and non-null

; Macro: CHECK_NAN_TRIPLE_32
; Checks 32-bit float NaN classification and returns triple in EAX
; Input: 32-bit float value in xmm0
; Output: NaN triple in EAX (bits 2:0)
%macro CHECK_NAN_TRIPLE_32 0
  push rcx
  push rsi
  push rdx
  
  xor eax, eax
  ucomiss xmm0, xmm0
  setp al
  lea rcx, [4*rax]
  
  ; Extract and check quiet bit (bit 22)
  movd edx, xmm0
  and edx, 0x00400000
  mov esi, edx
  shr esi, 22
  and sil, al
  movzx esi, sil
  lea rcx, [rcx + 2*rsi]
  
  ; Check for signaling NaN (NaN but not quiet)
  test edx, edx
  sete dl
  and dl, al
  movzx eax, dl
  or eax, ecx
  
  pop rdx
  pop rsi
  pop rcx
%endmacro

; Macro: CHECK_NAN_TRIPLE_64
; Checks 64-bit double NaN classification and returns triple in RAX
; Input: 64-bit double value in xmm0
; Output: NaN triple in RAX (bits 2:0)
%macro CHECK_NAN_TRIPLE_64 0
  push rcx
  push rsi
  push rdx
  
  xor eax, eax
  ucomisd xmm0, xmm0
  setp al
  lea rcx, [4*rax]
  
  ; Extract and check quiet bit (bit 51)
  movq rdx, xmm0
  mov rsi, 0x0008000000000000
  and rsi, rdx
  mov rdx, rsi
  shr rdx, 51
  and dl, al
  movzx rdx, dl
  lea rcx, [rcx + 2*rdx]
  
  ; Check for signaling NaN (NaN but not quiet)
  test rsi, rsi
  sete dl
  and dl, al
  movzx eax, dl
  or eax, ecx
  
  pop rdx
  pop rsi
  pop rcx
%endmacro

; Macro: CHECK_NAN_TRIPLE_80
; Checks 80-bit extended precision NaN classification and returns triple in RAX
; Input: 80-bit extended precision value in memory at [rax] (10 bytes)
; Output: NaN triple in RAX (bits 2:0)
%macro CHECK_NAN_TRIPLE_80 0
  push rcx
  push rdx
  push rsi
  
  ; Load the 80-bit value twice for comparison
  fld tword [rax]
  fld tword [rax]
  
  ; Store one copy to memory for bit manipulation
  sub rsp, 16
  fstp tword [rsp]
  
  ; Use fucomip for NaN detection
  xor eax, eax
  fucomip st0, st1
  setp al
  lea rdx, [4*rax]
  
  ; Extract and check quiet bit (bit 62)
  mov rcx, 0x4000000000000000
  and rcx, [rsp]
  mov rsi, rcx
  shr rsi, 62
  and sil, al
  movzx rsi, sil
  lea rdx, [rdx + 2*rsi]
  
  ; Check for signaling NaN (NaN but not quiet)
  test rcx, rcx
  sete cl
  and cl, al
  movzx eax, cl
  or eax, edx
  
  add rsp, 16
  pop rsi
  pop rdx
  pop rcx
%endmacro
