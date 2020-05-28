%ifdef CONFIG
{
  "RegData": {
    "MM6": ["0x8000000000000000", "0x3FFF"],
    "MM7": ["0x8000000000000000", "0x4000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov eax, 0x3f800000 ; 1.0
mov [rdx + 8 * 0], eax
mov eax, 0x40000000 ; 2.0
mov [rdx + 8 * 1], eax

fld dword [rdx + 8 * 0]
fld dword [rdx + 8 * 1]

fxch

hlt
