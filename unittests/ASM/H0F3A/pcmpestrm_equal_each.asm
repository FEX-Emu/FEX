%ifdef CONFIG
{
  "RegData": {
      "RAX":   ["8"],
      "RDX":   ["8"],
      "XMM1":  ["0x2121010101012121", "0x0000000001010101"],
      "XMM2":  ["0x306F8A9E672C65E5", "0x000030443057697D"],
      "XMM3":  ["0x306F8A9E672C65E5", "0x00003044305796E3"],
      "XMM4":  ["0x000000000000F43F", "0x0000000000000000"],
      "XMM5":  ["0x0000FFFFFFFFFFFF", "0xFFFFFFFF00FF0000"],
      "XMM6":  ["0x0000000000000BC0", "0x0000000000000000"],
      "XMM7":  ["0xFFFF000000000000", "0x00000000FF00FFFF"],
      "XMM8":  ["0x0000000000000BC0", "0x0000000000000000"],
      "XMM9":  ["0xFFFF000000000000", "0x00000000FF00FFFF"],
      "XMM10": ["0x00000000000000EF", "0x0000000000000000"],
      "XMM11": ["0xFFFFFFFFFFFFFFFF", "0xFFFFFFFFFFFF0000"],
      "XMM12": ["0x0000000000000010", "0x0000000000000000"],
      "XMM13": ["0x0000000000000000", "0x000000000000FFFF"],
      "XMM14": ["0x0000000000000010", "0x0000000000000000"],
      "XMM15": ["0x0000000000000000", "0x000000000000FFFF"]
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

; Full length unsigned byte string check (bits, positive polarity)
mov rax, 16
mov rdx, 16
CompareAndStore 0, 0b00001000, 4

; Full length unsigned byte string check (mask, positive polarity)
CompareAndStore 1, 0b01001000, 5

; Full length unsigned byte string check (bits, negative polarity)
CompareAndStore 2, 0b00011000, 6

; Full length unsigned byte string check (mask, negative polarity)
CompareAndStore 3, 0b01011000, 7

; Full length unsigned byte string check (bits, negative masked)
CompareAndStore 4, 0b00111000, 8

; Full length unsigned byte string check (mask, negative masked)
CompareAndStore 5, 0b01111000, 9

; --- 16-bit unsigned word tests ---

movaps xmm2, [rel .data16]
movaps xmm3, [rel .data16 + 32]

; Full length unsigned word string check (bits, positive polarity)
mov rax, 8
mov rdx, 8
CompareAndStore 6, 0b00001001, 10

; Full length unsigned word string check (mask, positive polarity)
CompareAndStore 7, 0b01001001, 11

; Full length unsigned word string check (bits, negative polarity)
CompareAndStore 8, 0b00011001, 12

; Full length unsigned word string check (mask, negative polarity)
CompareAndStore 9, 0b01011001, 13

; Full length unsigned word string check (bits, negative masked)
CompareAndStore 10, 0b00111001, 14

; Full length unsigned word string check (mask, negative masked)
CompareAndStore 11, 0b01111001, 15

; Load all our stored flags for result comparing
movaps xmm1, [rel .flags]

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

.flags:
dq 0x0000000000000000
dq 0x0000000000000000
dq 0x0000000000000000
dq 0x0000000000000000
