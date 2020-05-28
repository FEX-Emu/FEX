%ifdef CONFIG
{
  "RegData": {
    "MM7": ["0x8000000000000000", "0x4009"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov eax, 1024
mov [rdx + 8 * 0], eax
mov eax, -1
mov [rdx + 8 * 0 + 2], eax

fild word [rdx + 8 * 0]

hlt
