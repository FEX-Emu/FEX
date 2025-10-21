%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
      "XMM0": ["0x00060F000F000D01", "0x0000000000070007", "0x0000000000000000", "0x0000000000000000"],
      "XMM1": ["0x3111313131311111", "0x0000000000313131", "0x0000000000000000", "0x0000000000000000"],
      "XMM2": ["0x005A0041007A0061", "0x55AACCBBFF220000", "0xAAAAAAAAAAAAAAAA", "0xBBBBBBBBBBBBBBBB"],
      "XMM3": ["0x006500200027003F", "0x00210065004F0065", "0x8888888888888888", "0x9999999999999999"]
  }
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
; The second parameter is the control values to pass to vpcmpistri
;
%macro CompareAndStore 2
  vpcmpistri xmm2, xmm3, %2
  mov [rel .indices + %1], cl
  ArrangeAndStoreFLAGS %1
%endmacro

vmovaps ymm2, [rel .data]
vmovaps ymm3, [rel .data + 32]

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
vmovaps ymm2, [rel .data16]
vmovaps ymm3, [rel .data16 + 32]

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

; Load all our stored indices and flags for result comparing
vmovaps ymm0, [rel .indices]
vmovaps ymm1, [rel .flags]

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
