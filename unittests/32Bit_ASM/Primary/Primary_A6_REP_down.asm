%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x8300",
    "RCX": "0x9",
    "RDX": "0x0",
    "RDI": "0xE000000C",
    "RSI": "0xE000001C"
  },
  "Mode": "32BIT"
}
%endif

%macro copy 3
  ; Dest, Src, Size
  mov edi, %1
  mov esi, %2
  mov ecx, %3

  mov eax, 0x17
  mov es, eax
  mov ds, eax

  cld
  rep movsb
%endmacro

mov edx, 0xe0000000

lea ebx, [edx + 8 * 0]
lea ebp, .StringOne
copy ebx, ebp, 14

lea ebx, [edx + 8 * 2]
lea ebp, .StringTwo
copy ebx, ebp, 14

lea edi, [edx + 8 * 0 + 13]
lea esi, [edx + 8 * 2 + 13]

std
mov ecx, 10
repe cmpsb
mov eax, 0
lahf

mov edx, 0
sete dl

hlt

.StringOne: db "\0\0\0\0TestString"
.StringTwo: db "\0TestUnmatched"
