%ifdef CONFIG
{
  "Match": "All",
  "RegData": {
    "MM4": ["0xfffffffffffff800", "0xffff"],
    "MM5": ["0xffffff0000000000", "0xffff"],
    "MM6": ["0xaaaaaa0000000000", "0xbfd5"],
    "MM7": ["0xc90fdb0000000000", "0x4000"]
  }
}
%endif

lea rbp, [rel data]

; 32bit FLDs
fld dword [rbp]
fld dword [rbp + 4]
fld dword [rel allf] ; Currently fails due to lack of infinity handling

; 64bit FLDs
fld qword [rel allf] ; Currently fails due to lack of infinity handling

hlt

align 8
data:
        dd 0x40490fdb
        dd 0xaaaaaaaa
allf:   dq 0xffffffffffffffff
        dw 0xffff