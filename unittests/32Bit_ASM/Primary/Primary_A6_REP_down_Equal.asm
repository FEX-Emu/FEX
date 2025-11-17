%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4600",
    "RCX": "0x0",
    "RDX": "0x1",
    "RDI": "0xDFFFFFFF",
    "RSI": "0xE000000F"
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
copy ebx, ebp, 11

lea ebx, [edx + 8 * 2]
lea ebp, .StringTwo
copy ebx, ebp, 11

lea edi, [edx + 8 * 0 + 10]
lea esi, [edx + 8 * 2 + 10]

std
mov ecx, 11
repe cmpsb
mov eax, 0
lahf

mov edx, 0
sete dl

hlt

.StringOne: db "\0TestString"
.StringTwo: db "\0TestString"
