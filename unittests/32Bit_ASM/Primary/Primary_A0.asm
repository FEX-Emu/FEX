%ifdef CONFIG
{
  "RegData": {
    "RCX": "0xFFFF0042",
    "RDX": "0x42"
  },
  "Mode": "32BIT"
}
%endif

mov edx, 0xe0000000
mov eax, 0x42
mov [edx], eax

mov eax, -1
; mov eax, [0xe0000000]
db 0xA1
dd 0xe0000000
mov edx, eax

mov eax, -1
; mov ax, [0xe0000000]
db 0x66
db 0xA1
dd 0xe0000000
mov ecx, eax

; We can't actually test this one since we can't allocate memory in the lower 16bits
;mov eax, -1
;; mov ax, [0xe00]
;db 0x67
;db 0x66
;db 0xA1
;dw 0xe00
;mov ebx, eax

hlt
