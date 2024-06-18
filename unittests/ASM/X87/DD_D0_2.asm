%ifdef CONFIG
{
  "RegData": {
    "RAX": "1",
    "MM0": ["0x8000000000000000", "0x4000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

lea rdx, [rel data]
fld tword [rdx + 8 * 0]
fst st1  ;; copies st0, i.e. 2.0 to st1
fstp st0 ;; pop, st1 becomes st0

;; ensure st0 has valid tag.
fxam     ;; get if top is valid in C2
fstsw ax ;; store work into ax
shr ax, 10
and ax, 1

hlt

align 8
data:
  dt 2.0
  dq 0
