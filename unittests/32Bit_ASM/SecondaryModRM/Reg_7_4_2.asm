%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x0",
    "RCX": "0x000000000a121a20",
    "RDX": "0x000000000b131b20"
  },
  "Mode": "32BIT",
  "HostFeatures": ["CLZERO"]
}
%endif

; Starting address to store to
mov eax, 0xe8000000

; Set up the cachelines with garbage

; Cacheline 0
mov ebx, 0x41424344
mov [eax + 8 * 0], ebx
mov [eax + 8 * 1], ebx
mov [eax + 8 * 2], ebx
mov [eax + 8 * 3], ebx
mov [eax + 8 * 4], ebx
mov [eax + 8 * 5], ebx
mov [eax + 8 * 6], ebx
mov [eax + 8 * 7], ebx

; Cacheline 1
mov ebx, 0x55565758
mov [eax + 8 * 8], ebx
mov [eax + 8 * 9], ebx
mov [eax + 8 * 10], ebx
mov [eax + 8 * 11], ebx ; clzero here
mov [eax + 8 * 12], ebx
mov [eax + 8 * 13], ebx
mov [eax + 8 * 14], ebx
mov [eax + 8 * 15], ebx

; Cacheline 2
mov ebx, 0x61626364
mov [eax + 8 * 16], ebx
mov [eax + 8 * 17], ebx
mov [eax + 8 * 18], ebx
mov [eax + 8 * 19], ebx
mov [eax + 8 * 20], ebx
mov [eax + 8 * 21], ebx
mov [eax + 8 * 22], ebx
mov [eax + 8 * 23], ebx

; Set RAX to the middle of cacheline 1 to ensure alignment
lea eax, [eax + 8 * 11]

clzero

; Set eax back to the start
mov eax, 0xe8000000

mov ebx, 0
mov ecx, 0
mov edx, 0

; Cacheline 0 should be unmodified
add ecx, [eax + 8 * 0]
add ecx, [eax + 8 * 1]
add ecx, [eax + 8 * 2]
add ecx, [eax + 8 * 3]
add ecx, [eax + 8 * 4]
add ecx, [eax + 8 * 5]
add ecx, [eax + 8 * 6]
add ecx, [eax + 8 * 7]

; Cacheline 1 Should be zero
add ebx, [eax + 8 * 8]
add ebx, [eax + 8 * 9]
add ebx, [eax + 8 * 10]
add ebx, [eax + 8 * 11]
add ebx, [eax + 8 * 12]
add ebx, [eax + 8 * 13]
add ebx, [eax + 8 * 14]
add ebx, [eax + 8 * 15]

; Cacheline 2 should be unmodified
add edx, [eax + 8 * 16]
add edx, [eax + 8 * 17]
add edx, [eax + 8 * 18]
add edx, [eax + 8 * 19]
add edx, [eax + 8 * 20]
add edx, [eax + 8 * 21]
add edx, [eax + 8 * 22]
add edx, [eax + 8 * 23]

hlt
