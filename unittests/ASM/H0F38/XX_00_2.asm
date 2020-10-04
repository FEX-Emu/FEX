%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x4444444444444444",
    "MM1": "0x0"
  }
}
%endif

lea rdx, [rel .data]

movq mm0, [rdx + 8 * 0]
movq mm1, [rdx + 8 * 0]

pshufb mm0, [rdx + 8 * 1]
pshufb mm1, [rdx + 8 * 2]

hlt

align 8
.data:
; Incoming vector
dq 0x4142434445464748
; Test bits with trash data in reserved bits to ensure it is ignored
; Select single element
dq 0x7C7C7C7C7C7C7C7C
; Clear element
dq 0xF8F8F8F8F8F8F8F8
