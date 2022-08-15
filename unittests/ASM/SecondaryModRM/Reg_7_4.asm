%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x0"
  },
  "HostFeatures": ["CLZERO"]
}
%endif

; Starting address to store to
mov rax, 0xe8000000

; Set up the cacheline with garbage
mov rbx, 0x4142434445464748
mov [rax + 8 * 0], rbx
mov [rax + 8 * 1], rbx
mov [rax + 8 * 2], rbx
mov [rax + 8 * 3], rbx
mov [rax + 8 * 4], rbx
mov [rax + 8 * 5], rbx
mov [rax + 8 * 6], rbx
mov [rax + 8 * 7], rbx

clzero

mov rbx, 0

add rbx, [rax + 8 * 0]
add rbx, [rax + 8 * 1]
add rbx, [rax + 8 * 2]
add rbx, [rax + 8 * 3]
add rbx, [rax + 8 * 4]
add rbx, [rax + 8 * 5]
add rbx, [rax + 8 * 6]
add rbx, [rax + 8 * 7]

hlt
