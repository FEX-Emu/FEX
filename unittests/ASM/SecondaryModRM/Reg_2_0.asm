%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x3",
    "RDX": "0x0"
  },
  "HostFeatures": ["XSAVE"]
}
%endif

mov ecx, 0
xgetbv

; Mask only the lower two bits to get host and FEX runners to match.
; This way we can test that we're getting data back.
; Bit 0 and 1 refer to X87 and SSE respectively.
and eax, 0x3

hlt
