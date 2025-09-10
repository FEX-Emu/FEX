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
  push rcx                    ; Save rcx for temporary use
  push rsi                    ; Save rsi for temporary use
  push rdx                    ; Save rdx for temporary use
  
  xor eax, eax                ; Clear eax
  ucomiss xmm0, xmm0          ; Compare xmm0 with itself (unordered if NaN)
  setp al                     ; Set AL=1 if unordered (NaN detected)
  lea rcx, [4*rax]            ; Multiply by 4 to set bit 2 (NaN bit)
  
  ; Extract and check quiet bit (bit 22)
  movd edx, xmm0              ; Move float bits to integer register
  and edx, 0x00400000         ; Mask quiet bit (bit 22)
  mov esi, edx                ; Copy for manipulation
  shr esi, 22                 ; Shift quiet bit to position 0
  and sil, al                 ; AND with NaN flag (only valid if NaN)
  movzx esi, sil              ; Zero-extend to full register
  lea rcx, [rcx + 2*rsi]      ; Add 2*quiet_bit to result (sets bit 1 if quiet)
  
  ; Check for signaling NaN (NaN but not quiet)
  test edx, edx               ; Test if quiet bit was set
  sete dl                     ; Set DL=1 if quiet bit was NOT set
  and dl, al                  ; AND with NaN flag (signaling NaN detection)
  movzx eax, dl               ; Zero-extend signaling flag
  or eax, ecx                 ; Combine with previous result
  
  pop rdx                     ; Restore rdx
  pop rsi                     ; Restore rsi
  pop rcx                     ; Restore rcx
%endmacro

; Macro: CHECK_NAN_TRIPLE_64
; Checks 64-bit double NaN classification and returns triple in RAX
; Input: 64-bit double value in xmm0
; Output: NaN triple in RAX (bits 2:0)
%macro CHECK_NAN_TRIPLE_64 0
  push rcx                    ; Save rcx for temporary use
  push rsi                    ; Save rsi for temporary use
  push rdx                    ; Save rdx for temporary use
  
  xor eax, eax                ; Clear eax
  ucomisd xmm0, xmm0          ; Compare xmm0 with itself (unordered if NaN)
  setp al                     ; Set AL=1 if unordered (NaN detected)
  lea rcx, [4*rax]            ; Multiply by 4 to set bit 2 (NaN bit)
  
  ; Extract and check quiet bit (bit 51)
  movq rdx, xmm0              ; Move double bits to integer register
  mov rsi, 0x0008000000000000  ; Quiet bit mask (bit 51) = 2251799813685248
  and rsi, rdx                ; Mask quiet bit
  mov rdx, rsi                ; Copy for manipulation
  shr rdx, 51                 ; Shift quiet bit to position 0
  and dl, al                  ; AND with NaN flag (only valid if NaN)
  movzx rdx, dl               ; Zero-extend to full register
  lea rcx, [rcx + 2*rdx]      ; Add 2*quiet_bit to result (sets bit 1 if quiet)
  
  ; Check for signaling NaN (NaN but not quiet)
  test rsi, rsi               ; Test if quiet bit was set
  sete dl                     ; Set DL=1 if quiet bit was NOT set
  and dl, al                  ; AND with NaN flag (signaling NaN detection)
  movzx eax, dl               ; Zero-extend signaling flag
  or eax, ecx                 ; Combine with previous result
  
  pop rdx                     ; Restore rdx
  pop rsi                     ; Restore rsi
  pop rcx                     ; Restore rcx
%endmacro

; Macro: CHECK_NAN_TRIPLE_80
; Checks 80-bit extended precision NaN classification and returns triple in RAX
; Input: 80-bit extended precision value in memory at [rax] (10 bytes)
; Output: NaN triple in RAX (bits 2:0)
%macro CHECK_NAN_TRIPLE_80 0
  push rcx                    ; Save rcx for temporary use
  push rdx                    ; Save rdx for temporary use
  push rsi                    ; Save rsi for temporary use
  
  ; Load the 80-bit value twice for comparison
  fld tword [rax]             ; Load 80-bit value
  fld tword [rax]             ; Load same value again for comparison
  
  ; Store one copy to memory for bit manipulation
  sub rsp, 16                 ; Allocate space for 80-bit value  
  fstp tword [rsp]            ; Store copy to memory, pop stack
  
  ; Use fucomip for NaN detection (similar to compiler code)
  xor eax, eax                ; Clear eax
  fucomip st0, st1            ; Compare st0 with st1, pop stack
  setp al                     ; Set AL=1 if unordered (NaN detected)
  lea rdx, [4*rax]            ; Multiply by 4 to set bit 2 (NaN bit)
  
  ; Extract and check quiet bit (bit 62)
  mov rcx, 0x4000000000000000 ; Quiet bit mask (bit 62)
  and rcx, [rsp]              ; Mask quiet bit from stored significand
  mov rsi, rcx                ; Copy for manipulation
  shr rsi, 62                 ; Shift quiet bit to position 0
  and sil, al                 ; AND with NaN flag (only valid if NaN)
  movzx rsi, sil              ; Zero-extend to full register
  lea rdx, [rdx + 2*rsi]      ; Add 2*quiet_bit to result (sets bit 1 if quiet)
  
  ; Check for signaling NaN (NaN but not quiet)
  test rcx, rcx               ; Test if quiet bit was set
  sete cl                     ; Set CL=1 if quiet bit was NOT set
  and cl, al                  ; AND with NaN flag (signaling NaN detection)
  movzx eax, cl               ; Zero-extend signaling flag
  or eax, edx                 ; Combine with previous result
  
  add rsp, 16                 ; Clean up stack space
  pop rsi                     ; Restore rsi
  pop rdx                     ; Restore rdx
  pop rcx                     ; Restore rcx
%endmacro
