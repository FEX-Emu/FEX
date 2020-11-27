%ifdef CONFIG
{
  "RegData": {
    "MM6":  ["0x8000000000000000", "0x4001"],
    "MM7":  ["0x8000000000000000", "0x3FFF"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [.data]

fld qword [edx + 8 * 0]
fld qword [edx + 8 * 1]

; Undocumented x87 instruction
; Sets the tag register to empty for the stack register
; Then pops the stack
ffreep st0
fld qword [edx + 8 * 2] ; Overwrites previous value

hlt

.data:
dq 0x3ff0000000000000 ; 1.0
dq 0x4000000000000000 ; 2.0
dq 0x4010000000000000 ; 4.0


