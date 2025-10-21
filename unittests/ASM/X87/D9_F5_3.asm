%ifdef CONFIG
{
  "RegData": {
    "MM6":  ["0xC000000000000000", "0x4000"],
    "MM7":  ["0x8000000000000000", "0xC001"]
  }
}
%endif

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

lea rdx, [rel data2]
fld tword [rdx + 8 * 0]

fprem

lea rdx, [rel result1]
fstp tword [rdx + 8 * 0]

lea rdx, [rel data2]
fld tword [rdx + 8 * 0]

fprem1

lea rdx, [rel result2]
fstp tword [rdx + 8 * 0]

ffreep st0

lea rdx, [rel result1]
fld tword [rdx + 8 * 0]

lea rdx, [rel result2]
fld tword [rdx + 8 * 0]

; MM6 contains result2 (fprem1)
; MM7 contains result1 (fprem)

hlt

align 4096
data:
  dt 7.0
  dq 0
data2:
  dt -11.0
  dq 0

result1:
  dt 0.0
  dq 0.0
result2:
  dt 0.0
  dq 0.0
