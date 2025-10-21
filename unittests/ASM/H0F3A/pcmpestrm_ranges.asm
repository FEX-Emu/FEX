%ifdef CONFIG
{
  "RegData": {
      "RAX":   ["4"],
      "RDX":   ["16"],
      "XMM1":  ["0x3111313131311111", "0x0000000000313131"],
      "XMM2":  ["0x005A0041007A0061", "0x55AACCBBFF223344"],
      "XMM3":  ["0x006500200027003F", "0x00210065004F0065"],
      "XMM4":  ["0x0000000000003DEA", "0x0000000000000000"],
      "XMM5":  ["0xFFFFFF00FF00FF00", "0x0000FFFFFFFF00FF"],
      "XMM6":  ["0x000000000000C215", "0x0000000000000000"],
      "XMM7":  ["0x000000FF00FF00FF", "0xFFFF00000000FF00"],
      "XMM8":  ["0x000000000000C215", "0x0000000000000000"],
      "XMM9":  ["0x000000FF00FF00FF", "0xFFFF00000000FF00"],
      "XMM10": ["0xFFFF000000000000", "0x0000FFFFFFFFFFFF"],
      "XMM11": ["0x0000000000000087", "0x0000000000000000"],
      "XMM12": ["0x0000FFFFFFFFFFFF", "0xFFFF000000000000"],
      "XMM13": ["0x0000000000000087", "0x0000000000000000"],
      "XMM14": ["0x0000FFFFFFFFFFFF", "0xFFFF000000000000"]
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

; Performs the string comparison and moves the result from XMM0 to
; a specified vector in the third argument
;
; The first parameter is the location in memory result flags into.
; The second parameter is the control values to pass to pcmpistrm
; The third parameter is the XMM number to store the result in XMM0 to.
;
%macro CompareAndStore 3
  pcmpestrm xmm2, xmm3, %2
  movaps xmm%3, xmm0

  mov r15, rax
  ArrangeAndStoreFLAGS %1
  mov rax, r15
%endmacro

movaps xmm2, [rel .data]
movaps xmm3, [rel .data + 32]

; Range unsigned byte check (bits, positive polarity)
mov rax, 4
mov rdx, 16
CompareAndStore 0, 0b00000100, 4

; Range unsigned byte check (mask, positive polarity)
CompareAndStore 1, 0b01000100, 5

; Range unsigned byte check (bits, negative polarity)
CompareAndStore 2, 0b00010100, 6

; Range unsigned byte check (mask, negative polarity)
CompareAndStore 3, 0b01010100, 7

; Range unsigned byte check (bits, negative masked)
CompareAndStore 4, 0b00110100, 8

; Range unsigned byte check (mask, negative masked)
CompareAndStore 5, 0b01110100, 9

; --- 16-bit unsigned word tests ---
; Intentionally don't reset RDX to 8 here to test upper bounds clamping.
movaps xmm2, [rel .data16]
movaps xmm3, [rel .data16 + 32]

; Range unsigned word check (mask, positive polarity)
CompareAndStore 6, 0b01000101, 10

; Range unsigned word check (bits, negative polarity)
CompareAndStore 7, 0b00010101, 11

; Range unsigned word check (mask, negative polarity)
CompareAndStore 8, 0b01010101, 12

; Range unsigned word check (bits, negative masked)
CompareAndStore 9, 0b00110101, 13

; Range unsigned word check (mask, negative masked)
CompareAndStore 10, 0b01110101, 14

; Load all our stored flags for result comparing
movaps xmm1, [rel .flags]

hlt

align 4096
.data:
dq 0x998877665A417A61 ; "azAZ" (followed by junk)
dq 0x55AACCBBFF223344
dq 0xFFFFFFFFFFFFFFFF
dq 0xEEEEEEEEEEEEEEEE

dq 0x726548206D27493F ; "?I'm Her"
dq 0x21216E65704F2065 ; "e Open!!"
dq 0xFFFFFFFFFFFFFFFF
dq 0xEEEEEEEEEEEEEEEE

.data16:
dq 0x005A0041007A0061 ; "azAZ"
dq 0x55AACCBBFF223344
dq 0xAAAAAAAAAAAAAAAA
dq 0xBBBBBBBBBBBBBBBB

dq 0x006500200027003F ; "?' e"
dq 0x00210065004F0065 ; "eOen!"
dq 0x8888888888888888
dq 0x9999999999999999

.flags:
dq 0x0000000000000000
dq 0x0000000000000000
dq 0x0000000000000000
dq 0x0000000000000000
