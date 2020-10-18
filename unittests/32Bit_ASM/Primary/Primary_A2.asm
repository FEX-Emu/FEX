%ifdef CONFIG
{
  "RegData": {
    "RCX": "0x43",
    "RDX": "0x42"
  },
  "Mode": "32BIT"
}
%endif

mov eax, 0x42
; mov [0xe0000000], eax
db 0xA3
dd 0xe0000000

mov edx, 0xe0000000
mov edx, [edx]

mov eax, 0xFFFF0043
; mov [0xe0000000], ax
db 0x66
db 0xA3
dd 0xe0000000

mov ecx, 0xe0000000
mov ecx, [ecx]

; We can't actually test this one since we can't allocate memory in the lower 16bits
;mov eax, 0xFFFF0044
;; mov [0xe000], ax
;db 0x57
;db 0x66
;db 0xA3
;dw 0xe000
;
;mov ebx, 0xe000
;mov ebx, [ebx]

hlt
