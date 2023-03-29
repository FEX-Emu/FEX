%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000041D7FF00"
  },
  "Mode": "32BIT"
}
%endif

mov edx, 0xe0000000

mov eax, 0x41424344
mov [edx + 8 * 0], eax
mov eax, 0x51525354

; Set resulting al to zero
clc
salc
mov [edx + 8 * 0 + 0], al

; Set resulting al to 0xFF
stc
salc
lahf
mov [edx + 8 * 0 + 1], al

; Ensure that salc doesn't set flags
mov eax, -1
sahf
salc
lahf
mov [edx + 8 * 0 + 2], ah

mov eax, [edx + 8 * 0]

hlt
