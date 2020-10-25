%ifdef CONFIG
{
  "RegData": {
    "RSP": "0xE000001C"
  },
  "Mode": "32BIT"
}
%endif

mov esp, 0xe0000040

; Only push the segments
; Doesn't check for a correct segment value
; Just ensures we are pushing the correct amount of data
push cs
push ss
push ds
push es
push fs
push gs

o16 push cs
o16 push ss
o16 push ds
o16 push es
o16 push fs
o16 push gs

hlt
