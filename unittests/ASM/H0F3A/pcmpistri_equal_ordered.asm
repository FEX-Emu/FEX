%ifdef CONFIG
{
  "RegData": {
      "XMM0": ["0x05050F000F000902", "0x0000000006000700"],
      "XMM1": ["0x1919313131311111", "0x0000000039393939"],
      "XMM2": ["0x306F000030443057", "0x000030443057697D"],
      "XMM3": ["0x306F8A9E672C65E5", "0x00003044305796E3"]
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
; The second parameter is the control values to pass to pcmpistri
;
%macro CompareAndStore 2
  pcmpistri xmm2, xmm3, %2
  mov [rel .indices + %1], cl
  ArrangeAndStoreFLAGS %1
%endmacro

movaps xmm2, [rel .data]
movaps xmm3, [rel .data + 32]

; Unsigned byte string check (lsb, positive polarity)
CompareAndStore 0, 0b00001100

; Unsigned byte string check (msb, positive polarity)
CompareAndStore 1, 0b01001100

; Unsigned byte string check (lsb, negative polarity)
CompareAndStore 2, 0b00011100

; Unsigned byte string check (msb, negative polarity)
CompareAndStore 3, 0b01011100

; Unsigned byte string check (lsb, negative masked)
CompareAndStore 4, 0b00111100

; Unsigned byte string check (msb, negative masked)
CompareAndStore 5, 0b01111100

; --- 16-bit unsigned word tests ---
movaps xmm2, [rel .data16]
movaps xmm3, [rel .data16 + 32]

CompareAndStore 6, 0b00001101

; Unsigned word string check (msb, positive polarity)
CompareAndStore 7, 0b01001101

; Unsigned word string check (lsb, negative polarity)
CompareAndStore 8, 0b00011101

; Unsigned word string check (msb, negative polarity)
CompareAndStore 9, 0b01011101

; Unsigned word string check (lsb, negative masked)
CompareAndStore 10, 0b00111101

; Unsigned word string check (msb, negative masked)
CompareAndStore 11, 0b01111101

; Load all our stored indices and flags for result comparing
movaps xmm0, [rel .indices]
movaps xmm1, [rel .flags]

hlt

align 4096
.data:
dq 0x6550206F6F006C6C ; "ll" with junk following it
dq 0x21212121656C706F
dq 0xFFFFFFFFFFFFFFFF
dq 0xEEEEEEEEEEEEEEEE

dq 0x2759206F6C6C6548 ; "Hello Y'"
dq 0x21212121216C6C61 ; "all!!!!!"
dq 0xDDDDDDDDDDDDDDDD
dq 0xCCCCCCCCCCCCCCCC

.data16:
dq 0x306F000030443057 ; "しい" followed by junk
dq 0x000030443057697D
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
