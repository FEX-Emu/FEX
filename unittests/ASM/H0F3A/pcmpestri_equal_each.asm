%ifdef CONFIG
{
  "RegData": {
      "RAX": ["4"],
      "RDX": ["3"],
      "XMM0": ["0x0F000B060B060F00", "0x040407000F060706"],
      "XMM1": ["0x3939010101012121", "0x0101212119191919"],
      "XMM2": ["0x306F8A9E672C65E5", "0x000030443057697D"],
      "XMM3": ["0x306F8A9E672C65E5", "0x00003044305796E3"],
      "XMM4": ["0x0704030307000404", "0x0000000000000000"],
      "XMM5": ["0x1919191939390101", "0x0000000000000000"]
  },
  "HostFeatures": ["SSE4.2"]
}
%endif

; Adjusts the result from LAHF and SETO so that we have a set of flags organized
; like [OF, SF, ZF, AF, PF, CF] for storing into the .flags region
; of memory.
;
; The first parameter is the byte offset to store the flag result
; at in the .flags region of memory.
;
%macro ArrangeAndStoreFLAGS 1
  lahf
  seto bl
  movzx bx, bl

  shr ax, 8
  shl bx, 5

  mov di, ax
  mov si, ax

  ; Mask and shift
  and di, 0b0000_0000_0000_0100 ; PF
  and si, 0b0000_0000_0001_0000 ; AF
  shr di, 1
  shr si, 2

  ; OR all of them together
  or bx, di
  or bx, si

  ; Reclaim DI for getting ZF/SF and shift into place
  mov di, ax
  and di, 0b0000_0000_1100_0000 ; ZF and SF
  shr di, 3

  ; Finally mask and OR all of the bits together
  and ax, 0b0000_0000_0000_0001 ; CF
  or bx, ax
  or bx, di

  ; Store result to .flags memory
  mov [rel .flags + %1], bl
%endmacro

; Performs the string comparison and moves the result from RCX to
; a region of memory in the .indices section specified by a byte
; offset.
;
; The first parameter is the byte offset to store the RCX result to.
; The second parameter is the control values to pass to pcmpestri
;
%macro CompareAndStore 2
  pcmpestri xmm2, xmm3, %2
  mov [rel .indices + %1], cl

  mov r15, rax
  ArrangeAndStoreFLAGS %1
  mov rax, r15
%endmacro

movaps xmm2, [rel .data]
movaps xmm3, [rel .data + 32]

; Full length unsigned byte string check (lsb, positive polarity)
mov rax, 16
mov rdx, 16
CompareAndStore 0, 0b00001000

; Full length unsigned byte string check (msb, positive polarity)
CompareAndStore 1, 0b01001000

; Full length unsigned byte string check (lsb, negative polarity)
CompareAndStore 2, 0b00011000

; Full length unsigned byte string check (msb, negative polarity)
CompareAndStore 3, 0b01011000

; Full length unsigned byte string check (lsb, negative masked)
CompareAndStore 4, 0b00111000

; Full length unsigned byte string check (msb, negative masked)
CompareAndStore 5, 0b01111000

; Non-full length unsigned byte string check (lsb, positive polarity)
mov rax, 8
mov rdx, 7
CompareAndStore 6, 0b00001000

; Non-full length unsigned byte string check (msb, positive polarity)
CompareAndStore 7, 0b01001000

; Non-full length unsigned byte string check (lsb, negative polarity)
CompareAndStore 8, 0b00011000

; Non-full length unsigned byte string check (msb, negative polarity)
CompareAndStore 9, 0b01011000

; Non-full length unsigned byte string check (lsb, negative masked)
CompareAndStore 10, 0b00111000

; Non-full length unsigned byte string check (msb, negative masked)
CompareAndStore 11, 0b01111000

; --- 16-bit unsigned word tests ---

movaps xmm2, [rel .data16]
movaps xmm3, [rel .data16 + 32]

; Full length unsigned word string check (lsb, positive polarity)
mov rax, 8
mov rdx, 8
CompareAndStore 12, 0b00001001

; Full length unsigned word string check (msb, positive polarity)
CompareAndStore 13, 0b01001001

; Full length unsigned word string check (lsb, negative polarity)
CompareAndStore 14, 0b00011001

; Full length unsigned word string check (msb, negative polarity)
CompareAndStore 15, 0b01011001

; Full length unsigned word string check (lsb, negative masked)
CompareAndStore 16, 0b00111001

; Full length unsigned word string check (msb, negative masked)
CompareAndStore 17, 0b01111001

; Non-full length unsigned word string check (lsb, positive polarity)
mov rax, 4
mov rdx, 3
CompareAndStore 18, 0b00001001

; Non-full length unsigned word string check (msb, positive polarity)
CompareAndStore 19, 0b01001001

; Non-full length unsigned word string check (lsb, negative polarity)
CompareAndStore 20, 0b00011001

; Non-full length unsigned word string check (msb, negative polarity)
CompareAndStore 21, 0b01011001

; Non-full length unsigned word string check (lsb, negative masked)
CompareAndStore 22, 0b00111001

; Non-full length unsigned word string check (msb, negative masked)
CompareAndStore 23, 0b01111001

; Load all our stored indices and flags for result comparing
movaps xmm0, [rel .indices]
movaps xmm4, [rel .indices + 16]
movaps xmm1, [rel .flags]
movaps xmm5, [rel .flags + 16]

hlt

align 4096
.data:
dq 0x6550206F6C6C6548 ; "Hello Pe"
dq 0x21212121656C706F ; "ople!!!!"
dq 0xFFFFFFFFFFFFFFFF
dq 0xEEEEEEEEEEEEEEEE

dq 0x2759206F6C6C6548 ; "Hello Y'"
dq 0x21212121216C6C61 ; "all!!!!!"
dq 0xDDDDDDDDDDDDDDDD
dq 0xCCCCCCCCCCCCCCCC

.data16:
dq 0x306F8A9E672C65E5 ; "日本語は"
dq 0x000030443057697D ; "楽しい\0" (Japanese is fun)
dq 0xAAAAAAAAAAAAAAAA
dq 0xBBBBBBBBBBBBBBBB

dq 0x306F8A9E672C65E5 ; "日本語は"
dq 0x00003044305796E3 ; "難しい\0" (Japanese is hard)
dq 0x8888888888888888
dq 0x9999999999999999

.indices:
dq 0x0000000000000000
dq 0x0000000000000000
dq 0x0000000000000000
dq 0x0000000000000000

.flags:
dq 0x0000000000000000
dq 0x0000000000000000
dq 0x0000000000000000
dq 0x0000000000000000
