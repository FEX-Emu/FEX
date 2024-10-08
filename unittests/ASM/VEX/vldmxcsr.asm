%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "RAX": "0xFFC0"
  }
}
%endif

lea rdx, [rel .data]

; Currently we only implement setting the rounding mode and FTZ bit,
; so load junk into all the bits and check if we set the mode
;
; Result should be the default MXCSR (0x1F80) with the rounding
; mode bits (bits 13 and 14) and FTZ bit (bit 15) all set.
;
; Essentially just a small test to ensure we are indeed setting and saving
; the bits that we do emulate.

vldmxcsr [rdx]
vstmxcsr [rdx]
mov rax, [rdx]

hlt

align 4
.data:
dq 0x000000000000FFFF
