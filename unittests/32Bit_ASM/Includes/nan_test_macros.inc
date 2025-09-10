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
  push ecx                    ; Save ecx for temporary use
  push esi                    ; Save esi for temporary use
  push edx                    ; Save edx for temporary use
  
  xor eax, eax                ; Clear eax
  ucomiss xmm0, xmm0          ; Compare xmm0 with itself (unordered if NaN)
  setp al                     ; Set AL=1 if unordered (NaN detected)
  mov ecx, eax                ; Copy NaN flag
  shl ecx, 2                  ; Multiply by 4 to set bit 2 (NaN bit)
  
  ; Extract and check quiet bit (bit 22)
  movd edx, xmm0              ; Move float bits to integer register
  and edx, 0x00400000         ; Mask quiet bit (bit 22)
  mov esi, edx                ; Copy for manipulation
  shr esi, 22                 ; Shift quiet bit to position 0
  and esi, eax                ; AND with NaN flag (only valid if NaN)
  and esi, 1                  ; Ensure only bit 0 is preserved
  shl esi, 1                  ; Multiply quiet bit by 2
  add ecx, esi                ; Add 2*quiet_bit to result (sets bit 1 if quiet)
  
  ; Check for signaling NaN (NaN but not quiet)
  test edx, edx               ; Test if quiet bit was set
  sete dl                     ; Set DL=1 if quiet bit was NOT set
  and dl, al                  ; AND with NaN flag (signaling NaN detection)
  movzx eax, dl               ; Zero-extend signaling flag
  or eax, ecx                 ; Combine with previous result
  
  pop edx                     ; Restore edx
  pop esi                     ; Restore esi
  pop ecx                     ; Restore ecx
%endmacro

; Macro: CHECK_NAN_TRIPLE_64
; Checks 64-bit double NaN classification and returns triple in EAX
; Input: 64-bit double value should be pre-stored at [edx] by caller
; Output: NaN triple in EAX (bits 2:0)
%macro CHECK_NAN_TRIPLE_64 0
  push ebx                    ; Save ebx for temporary use
  push esi                    ; Save esi for temporary use
  push ecx                    ; Save ecx for temporary use
  
  ; Load 64-bit double from memory into x87 for comparison
  fld qword [edx]             ; Load double from [edx] into x87 stack
  fld st0                     ; Duplicate for comparison
  
  ; Use x87 comparison for NaN detection
  xor eax, eax                ; Clear eax
  fucom st0, st1              ; Compare duplicated values
  fnstsw ax                   ; Store status word
  sahf                        ; Store AH to flags
  setp al                     ; Set AL=1 if unordered (NaN detected)
  fstp st0                    ; Pop one value from stack
  fstp st0                    ; Pop other value from stack
  
  mov ecx, eax                ; Copy NaN flag
  shl ecx, 2                  ; Multiply by 4 to set bit 2 (NaN bit)
  
  ; Extract and check quiet bit (bit 51) - get high 32 bits
  mov ebx, 524288             ; Quiet bit mask (0x80000 for bit 19 in high dword)
  and ebx, [edx + 4]          ; Get quiet bit from high 32 bits
  
  mov esi, ebx                ; Copy for manipulation
  shr esi, 19                 ; Shift quiet bit to position 0 (bit 51-32=19)
  and si, ax                  ; AND with NaN flag (only valid if NaN)
  and esi, 1                  ; Keep only bit 0
  shl esi, 1                  ; Multiply quiet bit by 2
  add ecx, esi                ; Add 2*quiet_bit to result (sets bit 1 if quiet)
  
  ; Check for signaling NaN (NaN but not quiet)
  test ebx, ebx               ; Test if quiet bit was set
  sete bl                     ; Set BL=1 if quiet bit was NOT set
  and bl, al                  ; AND with NaN flag (signaling NaN detection)
  movzx eax, bl               ; Zero-extend signaling flag
  or eax, ecx                 ; Combine with previous result
  
  pop ecx                     ; Restore ecx
  pop esi                     ; Restore esi
  pop ebx                     ; Restore ebx
%endmacro

; Macro: CHECK_NAN_TRIPLE_80
; Checks 80-bit extended precision NaN classification and returns triple in EAX
; Input: 80-bit extended precision value in memory at [eax] (10 bytes)
; Output: NaN triple in EAX (bits 2:0)
%macro CHECK_NAN_TRIPLE_80 0
  push ebx                    ; Save ebx for temporary use
  push esi                    ; Save esi for temporary use
  
  ; Load the 80-bit value and store copy for bit manipulation
  fld tword [eax]             ; Load 80-bit value
  fld st0                     ; Duplicate for comparison
  sub esp, 20                 ; Allocate space for 80-bit value (20 bytes like compiler)
  fstp tword [esp]            ; Store copy to memory, pop stack
  
  ; Get bits 63:32 from stored significand (compiler template pattern)
  mov ecx, [esp + 4]          ; Get bits 63:32 from stored significand
  xor eax, eax                ; Clear eax
  fucomip st0, st0            ; Compare st0 with itself, pop stack
  setp al                     ; Set AL=1 if unordered (NaN detected)
  mov edx, eax                ; Copy NaN flag
  shl edx, 2                  ; Multiply by 4 to set bit 2 (NaN bit)
  
  ; Extract quiet bit (bit 31 in high dword, following compiler template)
  mov ebx, ecx                ; Copy for manipulation
  shr ebx, 31                 ; Shift bit 31 to position 0
  and bl, al                  ; AND with NaN flag (only valid if NaN)
  movzx esi, bl               ; Zero-extend quiet flag
  shl esi, 1                  ; Multiply quiet bit by 2
  add edx, esi                ; Add 2*quiet_bit to result (sets bit 1 if quiet)
  
  ; Check for signaling NaN using bt instruction (compiler template)
  bt ecx, 31                  ; Test bit 31 (quiet bit)
  setae cl                    ; Set CL=1 if carry flag NOT set (quiet bit NOT set)
  and cl, al                  ; AND with NaN flag (signaling NaN detection)
  movzx eax, cl               ; Zero-extend signaling flag
  or eax, edx                 ; Combine with previous result
  
  add esp, 20                 ; Clean up stack space
  pop esi                     ; Restore esi
  pop ebx                     ; Restore ebx
%endmacro