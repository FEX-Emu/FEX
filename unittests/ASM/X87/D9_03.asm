%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x3F800000",
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
mov eax, 0x0 ; 1.0
mov [rdx + 8 * 2], eax

fld dword [rdx + 8 * 0]
fstp dword [rdx + 8 * 2]
fld dword [rdx + 8 * 1]

mov eax, [rdx + 8 * 2]

hlt
