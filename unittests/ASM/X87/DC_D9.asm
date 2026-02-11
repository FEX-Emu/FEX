%ifdef CONFIG
{
  "RegData": {
    "MM6":  ["0x8000000000000000", "0x3FFF"],
    "MM7":  ["0x8000000000000000", "0x3FFF"]
  }
}
%endif

; Only tests pop behaviour
; Tests undocumented fcomp implementation at 0xdc, 0xd8+i
finit
fld1
fldz
; fcomp
db 0xdc, 0xd9
fld1

hlt
