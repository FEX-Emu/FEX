%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x0",
    "RCX": "0x000000020A121A20",
    "RDX": "0x0A121A2000000000"
  },
  "HostFeatures": ["CLZERO"]
}
%endif

; Starting address to store to
mov rax, 0xe8000000

; Set up the cachelines with garbage

; Cacheline 0
mov rbx, 0x0000000041424344
mov [rax + 8 * 0], rbx
mov [rax + 8 * 1], rbx
mov [rax + 8 * 2], rbx
mov [rax + 8 * 3], rbx
mov [rax + 8 * 4], rbx
mov [rax + 8 * 5], rbx
mov [rax + 8 * 6], rbx
mov [rax + 8 * 7], rbx

; Cacheline 1
mov rbx, 0x5152535455565758
mov [rax + 8 * 8], rbx
mov [rax + 8 * 9], rbx
mov [rax + 8 * 10], rbx
mov [rax + 8 * 11], rbx ; clzero here
mov [rax + 8 * 12], rbx
mov [rax + 8 * 13], rbx
mov [rax + 8 * 14], rbx
mov [rax + 8 * 15], rbx

; Cacheline 2
mov rbx, 0x4142434400000000
mov [rax + 8 * 16], rbx
mov [rax + 8 * 17], rbx
mov [rax + 8 * 18], rbx
mov [rax + 8 * 19], rbx
mov [rax + 8 * 20], rbx
mov [rax + 8 * 21], rbx
mov [rax + 8 * 22], rbx
mov [rax + 8 * 23], rbx

; Set RAX to the middle of cacheline 1 to ensure alignment
lea rax, [rax + 8 * 11]

clzero

; Set rax back to the start
mov rax, 0xe8000000

mov rbx, 0
mov rcx, 0
mov rdx, 0

; Cacheline 0 should be unmodified
add rcx, [rax + 8 * 0]
add rcx, [rax + 8 * 1]
add rcx, [rax + 8 * 2]
add rcx, [rax + 8 * 3]
add rcx, [rax + 8 * 4]
add rcx, [rax + 8 * 5]
add rcx, [rax + 8 * 6]
add rcx, [rax + 8 * 7]

; Cacheline 1 Should be zero
add rbx, [rax + 8 * 8]
add rbx, [rax + 8 * 9]
add rbx, [rax + 8 * 10]
add rbx, [rax + 8 * 11]
add rbx, [rax + 8 * 12]
add rbx, [rax + 8 * 13]
add rbx, [rax + 8 * 14]
add rbx, [rax + 8 * 15]

; Cacheline 2 should be unmodified
add rdx, [rax + 8 * 16]
add rdx, [rax + 8 * 17]
add rdx, [rax + 8 * 18]
add rdx, [rax + 8 * 19]
add rdx, [rax + 8 * 20]
add rdx, [rax + 8 * 21]
add rdx, [rax + 8 * 22]
add rdx, [rax + 8 * 23]

hlt
