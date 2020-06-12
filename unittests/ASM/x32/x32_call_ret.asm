%ifdef CONFIG
{
  "Mode": "32BIT",
  "RegData": {
    "RAX": "0xFFFFFFFF"
  }
}
%endif
BITS 32;

mov esp, 0xe8000000

mov eax, 0
call .label

hlt

.label:
mov eax, -1
ret

