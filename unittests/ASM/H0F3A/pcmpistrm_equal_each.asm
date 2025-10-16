%ifdef CONFIG
{
  "RegData": {
      "XMM1":  ["0x3939191919193939", "0x0000000019191919"],
      "XMM2":  ["0x306F8A9E672C65E5", "0x000030443057697D"],
      "XMM3":  ["0x306F8A9E672C65E5", "0x00003044305796E3"],
      "XMM4":  ["0x000000000000B43F", "0x0000000000000000"],
      "XMM5":  ["0x0000FFFFFFFFFFFF", "0xFF00FFFF00FF0000"],
      "XMM6":  ["0x0000000000004BC0", "0x0000000000000000"],
      "XMM7":  ["0xFFFF000000000000", "0x00FF0000FF00FFFF"],
      "XMM8":  ["0x000000000000CBC0", "0x0000000000000000"],
      "XMM9":  ["0xFFFF000000000000", "0xFFFF0000FF00FFFF"],
      "XMM10": ["0x00000000000000EF", "0x0000000000000000"],
      "XMM11": ["0xFFFFFFFFFFFFFFFF", "0xFFFFFFFFFFFF0000"],
      "XMM12": ["0x0000000000000010", "0x0000000000000000"],
      "XMM13": ["0x0000000000000000", "0x000000000000FFFF"],
      "XMM14": ["0x0000000000000090", "0x0000000000000000"],
      "XMM15": ["0x0000000000000000", "0xFFFF00000000FFFF"]
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
; a region of memory in the .indices section specified by a byte
; offset.
;
; The first parameter is the location in memory result flags into.
; The second parameter is the control values to pass to pcmpistrm
; The third parameter is the XMM number to store the result in XMM0 to.
;
%macro CompareAndStore 3
  pcmpistrm xmm2, xmm3, %2
  movaps xmm%3, xmm0
  ArrangeAndStoreFLAGS %1
%endmacro

movaps xmm2, [rel .data]
movaps xmm3, [rel .data + 32]

; Unsigned byte string check (bits, positive polarity)
CompareAndStore 0, 0b00001000, 4

; Unsigned byte string check (mask, positive polarity)
CompareAndStore 1, 0b01001000, 5

; Unsigned byte string check (bits, negative polarity)
CompareAndStore 2, 0b00011000, 6

; Unsigned byte string check (mask, negative polarity)
CompareAndStore 3, 0b01011000, 7

; Unsigned byte string check (bits, negative masked)
CompareAndStore 4, 0b00111000, 8

; Unsigned byte string check (mask, negative masked)
CompareAndStore 5, 0b01111000, 9

; --- 16-bit unsigned word tests ---

movaps xmm2, [rel .data16]
movaps xmm3, [rel .data16 + 32]

; Unsigned word string check (bits, positive polarity)
CompareAndStore 6, 0b00001001, 10

; Unsigned word string check (mask, positive polarity)
CompareAndStore 7, 0b01001001, 11

; Unsigned word string check (bits, negative polarity)
CompareAndStore 8, 0b00011001, 12

; Unsigned word string check (mask, negative polarity)
CompareAndStore 9, 0b01011001, 13

; Unsigned word string check (bits, negative masked)
CompareAndStore 10, 0b00111001, 14

; Unsigned word string check (mask, negative masked)
CompareAndStore 11, 0b01111001, 15

; Load all our stored flags for result comparing
movaps xmm1, [rel .flags]

hlt

align 4096
.data:
dq 0x6550206F6C6C6548 ; "Hello Pe"
dq 0x00002121656C706F ; "ople!!\0\0"
dq 0xFFFFFFFFFFFFFFFF
dq 0xEEEEEEEEEEEEEEEE

dq 0x2759206F6C6C6548 ; "Hello Y'"
dq 0x00212121216C6C61 ; "all!!!!\0"
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

.flags:
dq 0x0000000000000000
dq 0x0000000000000000
dq 0x0000000000000000
dq 0x0000000000000000
