%ifdef CONFIG
{
  "RegData": {
      "XMM0": ["0x00060F000F000D01", "0x0000001010070007"],
      "XMM1": ["0x3111313131311111", "0x0000001818313131"],
      "XMM2": ["0x005A0041007A0061", "0x55AACCBBFF220000"],
      "XMM3": ["0x0065002000270000", "0x00210065004F0065"]
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

; Range unsigned byte check (lsb, positive polarity)
CompareAndStore 0, 0b00000100

; Range unsigned byte check (msb, positive polarity)
CompareAndStore 1, 0b01000100

; Range unsigned byte check (lsb, negative polarity)
CompareAndStore 2, 0b00010100

; Range unsigned byte check (msb, negative polarity)
CompareAndStore 3, 0b01010100

; Range unsigned byte check (lsb, negative masked)
CompareAndStore 4, 0b00110100

; Range unsigned byte check (msb, negative masked)
CompareAndStore 5, 0b01110100

; --- 16-bit unsigned word tests ---
movaps xmm2, [rel .data16]
movaps xmm3, [rel .data16 + 32]

; Range unsigned word check (msb, positive polarity)
CompareAndStore 6, 0b01000101

; Range unsigned word check (lsb, negative polarity)
CompareAndStore 7, 0b00010101

; Range unsigned word check (msb, negative polarity)
CompareAndStore 8, 0b01010101

; Range unsigned word check (lsb, negative masked)
CompareAndStore 9, 0b00110101

; Range unsigned word check (msb, negative masked)
CompareAndStore 10, 0b01110101

; --- Edge case test (string begins with null character) ---
movaps xmm2, [rel .data_null]
movaps xmm3, [rel .data_null + 32]

; Range signed byte check (msb)
CompareAndStore 11, 0b01000110

; Range signed byte check (lsb)
CompareAndStore 12, 0b01000110

; Load all our stored indices and flags for result comparing
movaps xmm0, [rel .indices]
movaps xmm1, [rel .flags]

hlt

align 4096
.data:
dq 0x998877005A417A61 ; "azAZ" (followed by junk)
dq 0x55AACCBBFF223344
dq 0xFFFFFFFFFFFFFFFF
dq 0xEEEEEEEEEEEEEEEE

dq 0x726548206D27493F ; "?I'm Her"
dq 0x21216E65704F2065 ; "e Open!!"
dq 0xFFFFFFFFFFFFFFFF
dq 0xEEEEEEEEEEEEEEEE

.data16:
dq 0x005A0041007A0061 ; "azAZ"
dq 0x55AACCBBFF220000
dq 0xAAAAAAAAAAAAAAAA
dq 0xBBBBBBBBBBBBBBBB

dq 0x006500200027003F ; "?' e"
dq 0x00210065004F0065 ; "eOen!"
dq 0x8888888888888888
dq 0x9999999999999999

.data_null:
dq 0x005A0041007A0061 ; "azAZ"
dq 0x55AACCBBFF220000
dq 0xAAAAAAAAAAAAAAAA
dq 0xBBBBBBBBBBBBBBBB

dq 0x0065002000270000 ; "\0' e"
dq 0x00210065004F0065 ; "eOen!"
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
