%ifdef CONFIG
{
  "RegData": {
    "R15": "0xFFFFFFFFFFFFFFFF",
    "R14": "0x4141414141410000",
    "R13": "0",
    "R12": "0",
    "R11": "0x00000000FFFFFFFF"
  }
}
%endif

mov r15, -1
mov r14, 0x4141414141410000
mov r13, 0
mov r12, 0
mov r11, -1

shld r14w, r15w, 0
shld r13d, r15d, 0
shld r12, r15, 0
shld r11d, r15d, 0

hlt
