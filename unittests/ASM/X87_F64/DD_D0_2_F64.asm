%ifdef CONFIG
{
  "RegData": {
    "RAX": "1",
    "RBX": "0x4000000000000000"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

lea rdx, [rel data]
fld qword [rdx]
fst st1  ;; copies st0, i.e. 2.0 to st1
fstp st0 ;; pop, st1 becomes st0

;; ensure st0 has valid tag.
fxam     ;; get if top is valid in C2
fstsw ax ;; store work into ax
shr ax, 10
and ax, 1

; store top in rbx
fst qword [rdx]
mov rbx, [rdx]

hlt

align 8
data:
  dq 2.0