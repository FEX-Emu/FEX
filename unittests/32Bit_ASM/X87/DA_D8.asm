%ifdef CONFIG
{
  "RegData": {
    "MM5":  ["0x8000000000000000", "0x3FFF"],
    "MM6":  ["0x0000000000000000", "0x0000"],
    "MM7":  ["0x8000000000000000", "0x3FFF"]
  },
  "Mode": "32BIT"
}
%endif

fld1
fldz

mov eax, 0x0
cmp eax, -1

fcmovu st0, st1

fldz
cmp eax, 1
fcmovu st0, st2

hlt
