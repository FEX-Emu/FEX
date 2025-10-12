%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445460000",
    "RBX": "0x0",
    "RDX": "1",
    "R9": "1",
    "R10": "1"
  },
  "HostFeatures": ["RAND"]
}
%endif

mov rax, 0x4142434445464748
mov rbx, 0x4142434445464748
mov rcx, 0x4142434445464748

; 16-bit should insert
test_16bit:
rdseed ax
jnc test_16bit

; Mask out RNG
mov r11, 0xFFFFFFFFFFFF0000
and rax, r11

mov r8, 0x4142434445460000
cmp rax, r8

mov rdx, 0
sete dl

; 32-bit and 64-bit should zext
test_32bit:
rdseed ebx
jnc test_32bit

; Mask out RNG
mov r11, 0xFFFFFFFF00000000
and rbx, r11

mov r8, 0x4142434400000000
cmp r11, r8

mov r9, 0
setne r9b

test_64bit:
rdseed rcx
jnc test_64bit

mov r8, 0x0
cmp rcx, r8

mov r10, 0
setne r10b

hlt
