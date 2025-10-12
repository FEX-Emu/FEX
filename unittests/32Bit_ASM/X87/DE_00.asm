%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0xc000000000000000", "0x4000"],
    "XMM1":  ["0x8000000000000000", "0xbfff"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [rel .data]

fld qword [edx + 8 * 0]
fiadd word [edx + 8 * 1]
fstp tword [rel data2]
movups xmm0, [rel data2]

; Test negative
lea edx, [rel .data_neg]

fld qword [edx + 8 * 0]
fiadd word [edx + 8 * 1]

fstp tword [rel data2]

movups xmm1, [rel data2]

hlt

align 4096
.data:
dq 0x3ff0000000000000
dq 2

.data_neg:
dq 0x3ff0000000000000
dq -2

data2:
dq 0
dq 0
