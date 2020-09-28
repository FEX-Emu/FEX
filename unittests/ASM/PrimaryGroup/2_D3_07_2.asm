%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x3335785C36350000",
    "RBX": "0x3465785C33350000",
    "RCX": "0x6663785C34650332",
    "RDX": "0x3234785C66630F0B",
    "RDI": "0x3234785C32340000",
    "RSI": "0x3035785C32340000",
    "RSP": "0x3564785C30350000",
    "R8":  "0x6532785C35640785",
    "R9":  "0x6435785C65320000",
    "R10": "0x3262785C64350000",
    "R11": "0x6638785C32621E17",
    "R12": "0x3831785C66380000",
    "R13": "0x3434785C38310000",
    "R14": "0x6632785C34340000",
    "R15": "0x3162785C66320000"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

lea r15, [rel .data]

mov rax, [r15 + 0]
mov cl, [r15 + 2]
sar ax, cl

mov rbx, [r15 + 4]
mov cl, [r15 + 6]
sar bx, cl

mov rcx, [r15 + 8]
mov cl, [r15 + 10]
sar cx, cl

mov rdx, [r15 + 12]
mov cl, [r15 + 14]
sar dx, cl

mov rdi, [r15 + 16]
mov cl, [r15 + 18]
sar di, cl

mov rsi, [r15 + 20]
mov cl, [r15 + 22]
sar si, cl

mov rsp, [r15 + 24]
mov cl, [r15 + 26]
sar sp, cl

mov r8, [r15 + 28]
mov cl, [r15 + 30]
sar r8w, cl

mov r9, [r15 + 32]
mov cl, [r15 + 34]
sar r9w, cl

mov r10, [r15 + 36]
mov cl, [r15 + 38]
sar r10w, cl

mov r11, [r15 + 40]
mov cl, [r15 + 42]
sar r11w, cl

mov r12, [r15 + 44]
mov cl, [r15 + 46]
sar r12w, cl

mov r13, [r15 + 48]
mov cl, [r15 + 50]
sar r13w, cl

mov r14, [r15 + 52]
mov cl, [r15 + 54]
sar r14w, cl

mov cl, [r15 + 58]
mov r15, [r15 + 56]
sar r15w, cl

hlt

.data:
db '\x56\x53\xe4\xcf\x42\x42\x50\xd5\x2e\x5d\xb2\x8f\x18\x44\x2f\xb1'
db '\xad\x88\x64\x7e\x20\x99\xb4\xf8\xa4\x34\xc7\x65\xd7\x01\x19\xc3'
db '\x8c\xce\x28\x7c\x64\x65\x50\x65\xb7\xda\xaf\x08\xc0\x1f\x31\xbf'
db '\x7f\xeb\xf0\x0b\xf0\x46\x4e\x72\x2c\xf8\xb4\x4b\xa9\x8d\xc9\x33'
