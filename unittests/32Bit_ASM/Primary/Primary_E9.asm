%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x41424344"
  },
  "Mode": "32BIT"
}
%endif

; Tests for 32-bit signed displacement wrapping
; Testing for underflow specifically
; Will crash or hit the code we emit to memory

; We map ten pages to 0xe000'0000
; Generate a mov eax + hlt over there first
; 0xb8'44'43'42'41: mov eax, 0x41424344
; 0xf4: hlt

mov ebx, 0xe0000000
mov al, 0xb8
mov byte [ebx], al
mov eax, 0x41424344
mov dword [ebx + 1], eax
mov al, 0xf4
mov byte [ebx + 5], al

; Do a jump dance to stop multiblock from trying to optimize
; Otherwise it will JIT code from 0xe000'0000 before written
lea ebx, [rel next]
jmp ebx
next:

; Move temp to eax to overwrite
mov eax, 0

; This is dependent on where it is in the code!
jmp -0x20000000

; Definitely wrong if we hit here
mov eax, -1
hlt
