; NaN Testing Macros for 32-bit Assembly Tests
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
; ASSUMPTION: All input pointers (edx) are valid and non-null

; Macro: CHECK_NAN_TRIPLE_32
; Checks 32-bit float NaN classification and returns triple in EAX
; Input: 32-bit float value in xmm0
; Output: NaN triple in EAX (bits 2:0)
%macro CHECK_NAN_TRIPLE_32 0
  push ecx
  push esi
  push edx
  
  xor eax, eax
  ucomiss xmm0, xmm0
  setp al
  mov ecx, eax
  shl ecx, 2
  
  ; Extract and check quiet bit (bit 22)
  movd edx, xmm0
  and edx, 0x00400000
  mov esi, edx
  shr esi, 22
  and esi, eax
  and esi, 1
  shl esi, 1
  add ecx, esi
  
  ; Check for signaling NaN (NaN but not quiet)
  test edx, edx
  sete dl
  and dl, al
  movzx eax, dl
  or eax, ecx
  
  pop edx
  pop esi
  pop ecx
%endmacro

; Macro: CHECK_NAN_TRIPLE_64
; Checks 64-bit double NaN classification and returns triple in EAX
; Input: 64-bit double value should be pre-stored at [edx] by caller
; Output: NaN triple in EAX (bits 2:0)
%macro CHECK_NAN_TRIPLE_64 0
  push ebx
  push esi
  sub esp, 12
  
  ; Load 64-bit double and use SSE for NaN comparison
  movsd xmm0, qword [edx]
  xor eax, eax
  ucomisd xmm0, xmm0
  setp al
  
  mov ecx, eax
  movsd qword [esp], xmm0
  mov edx, 524288
  and edx, [esp + 4]
  shl ecx, 2
  
  ; Extract quiet bit (bit 51)
  mov ebx, edx
  shr ebx, 19
  and bl, al
  movzx esi, bl
  lea ecx, [ecx + 2*esi]
  
  ; Check for signaling NaN (NaN but not quiet)
  test edx, edx
  sete dl
  and dl, al
  movzx eax, dl
  or eax, ecx
  
  add esp, 12
  pop esi
  pop ebx
%endmacro

; Macro: CHECK_NAN_TRIPLE_80
; Checks 80-bit extended precision NaN classification and returns triple in EAX
; Input: 80-bit extended precision value in memory at [eax] (10 bytes)
; Output: NaN triple in EAX (bits 2:0)
%macro CHECK_NAN_TRIPLE_80 0
  push ebx
  push esi
  sub esp, 20
  
  ; Load the 80-bit value and store copy for bit manipulation
  fld tword [eax]
  fld st0
  fstp tword [esp]
  
  ; Get bits 63:32 from stored significand
  mov ecx, [esp + 4]
  xor eax, eax
  fucomip st0
  setp al
  mov edx, eax
  shl edx, 2
  
  ; Extract quiet bit (bit 30 in high dword)
  mov ebx, ecx
  shr ebx, 30
  and bl, al
  movzx esi, bl
  lea edx, [edx + 2*esi]
  
  ; Check for signaling NaN using bt instruction
  bt ecx, 30
  setae cl
  and cl, al
  movzx eax, cl
  or eax, edx
  
  add esp, 20
  pop esi
  pop ebx
%endmacro
