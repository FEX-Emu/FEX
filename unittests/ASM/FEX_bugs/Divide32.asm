%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x80000001",
    "RDX": "0x1"
  }
}
%endif

; FEX had a bug where we failed to ignore garbage upper bits of a 32-bit divisor
; with div. This test does a division with garbage in the upper bits where the
; result would differ if they were not ignored.

; 0x100000003 / 0x2 = 0x80000001 remainder 1
mov edx, 1
mov eax, 3
mov rcx, 0xdeadbeef00000002
div ecx

hlt
