%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x2",
    "MM7": ["0x8000000000000000", "0x3FFF"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [rel data]
fld tword [edx + 8 * 0]

lea edx, [rel data3]
fisttp word [edx + 8 * 0]

mov ax, word [edx + 8 * 0]

lea edx, [rel data2]
fld tword [edx + 8 * 0]

hlt

align 4096
data:
  dt 2.0
  dq 0
data2:
  dt 1.0
  dq 0
data3:
  dq -1
  dq -1
