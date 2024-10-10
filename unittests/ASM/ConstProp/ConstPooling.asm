%ifdef CONFIG
{
  "Match": "All",
  "RegData": {
    "RAX": "0xf"
  }
}
%endif

mov rdx, 0xe0000000

; generate a lot of re-used constants
mov dword[rdx], 0x0
mov dword[rdx], 0x1
mov dword[rdx], 0x2
mov dword[rdx], 0x3
mov dword[rdx], 0x4
mov dword[rdx], 0x5
mov dword[rdx], 0x6
mov dword[rdx], 0x7
mov dword[rdx], 0x8
mov dword[rdx], 0x9
mov dword[rdx], 0xa
mov dword[rdx], 0xb
mov dword[rdx], 0xc
mov dword[rdx], 0xd
mov dword[rdx], 0xe
mov dword[rdx], 0xf
mov dword[rdx], 0x0
mov dword[rdx], 0x1
mov dword[rdx], 0x2
mov dword[rdx], 0x3
mov dword[rdx], 0x4
mov dword[rdx], 0x5
mov dword[rdx], 0x6
mov dword[rdx], 0x7
mov dword[rdx], 0x8
mov dword[rdx], 0x9
mov dword[rdx], 0xa
mov dword[rdx], 0xb
mov dword[rdx], 0xc
mov dword[rdx], 0xd
mov dword[rdx], 0xe
mov dword[rdx], 0xf
mov dword[rdx], 0x0
mov dword[rdx], 0x1
mov dword[rdx], 0x2
mov dword[rdx], 0x3
mov dword[rdx], 0x4
mov dword[rdx], 0x5
mov dword[rdx], 0x6
mov dword[rdx], 0x7
mov dword[rdx], 0x8
mov dword[rdx], 0x9
mov dword[rdx], 0xa
mov dword[rdx], 0xb
mov dword[rdx], 0xc
mov dword[rdx], 0xd
mov dword[rdx], 0xe
mov dword[rdx], 0xf
mov dword[rdx], 0x0
mov dword[rdx], 0x1
mov dword[rdx], 0x2
mov dword[rdx], 0x3
mov dword[rdx], 0x4
mov dword[rdx], 0x5
mov dword[rdx], 0x6
mov dword[rdx], 0x7
mov dword[rdx], 0x8
mov dword[rdx], 0x9
mov dword[rdx], 0xa
mov dword[rdx], 0xb
mov dword[rdx], 0xc
mov dword[rdx], 0xd
mov dword[rdx], 0xe
mov dword[rdx], 0xf

mov eax, dword[rdx]

hlt