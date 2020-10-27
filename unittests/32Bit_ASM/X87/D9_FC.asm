%ifdef CONFIG
{
  "RegData": {
    "MM7": ["0x8000000000000000", "0x3fff"]
  },
  "Mode": "32BIT"
}
%endif

mov edx, 0xe0000000

mov eax, 0x3f834241 ; 1.02546
mov [edx + 8 * 0], eax

fld dword [edx + 8 * 0]

frndint

hlt
