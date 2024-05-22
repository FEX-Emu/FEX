%ifdef CONFIG
{
  "MemoryRegions": {
    "0x200000000": "0x20000"
  }
}
%endif

; FEX has had various bugs throughout the years leading to accidental
; superlinear time, for example with constant pooling and register allocation.
; This test mimics the massive block found in XeSS. If this test has
; excessive runtime, something in broken in FEXCore.

mov rsp, 0x200000000
%assign i 0
%rep 0x10000
mov byte [rsp + (0x10000 + i)], (0x01 + (i << 2)) & 0xFF
%assign i i+1
%endrep

hlt
