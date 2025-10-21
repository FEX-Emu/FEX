%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x00000000",
    "RBX": "0x40000000",
    "MM7": ["0x8000000000000000", "0x3FFF"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [rel data]
fld tword [edx + 8 * 0]

lea edx, [rel data3]
fstp qword [edx + 8 * 0]

mov eax, [edx + 4 * 0]
mov ebx, [edx + 4 * 1]

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
  dq 0
  dq 0
