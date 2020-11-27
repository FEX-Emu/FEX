%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x00000000",
    "RBX": "0x40000000",
    "MM6": ["0x8000000000000000", "0x3FFF"],
    "MM7": ["0x8000000000000000", "0x4000"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [data]
fld tword [edx + 8 * 0]

lea edx, [data3]
fst qword [edx + 8 * 0]

mov eax, [edx + 4 * 0]
mov ebx, [edx + 4 * 1]

lea edx, [data2]
fld tword [edx + 8 * 0]

hlt

align 8
data:
  dt 2.0
  dq 0
data2:
  dt 1.0
  dq 0
data3:
  dq 0
  dq 0
