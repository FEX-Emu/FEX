%ifdef CONFIG
{
  "RegData": {
    "MM7":  ["0xD76AA47848677021", "0x3FFE"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [data]
fld tword [edx + 8 * 0]

fsin

hlt

align 8
data:
  dt 1.0
  dq 0
