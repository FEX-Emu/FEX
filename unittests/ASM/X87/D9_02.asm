%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x3F800000",
    "MM7": ["0x8000000000000000", "0x3fff"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov eax, 0x3f800000 ; 1.0
mov [rdx + 8 * 0], eax
mov eax, 0x0
mov [rdx + 8 * 1], eax

fld dword [rdx + 8 * 0]
fst dword [rdx + 8 * 1]

mov eax, [rdx + 8 * 1]

hlt
