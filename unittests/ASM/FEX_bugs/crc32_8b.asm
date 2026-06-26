%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x9CB095B0",
    "RCX": "0x0000000011223344"
  }
}
%endif

mov eax, 0xFFFFFFFF
mov rcx, 0x11223344

crc32 eax, cl

crc32 eax, ch

hlt