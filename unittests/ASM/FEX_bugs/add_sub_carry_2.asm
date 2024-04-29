%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xedededee26260e6c",
    "RBX": "0x121212129498c16d"
  }
}
%endif

; FEX had a bug with smaller than 32-bit operations corrupting sbb and adc results.
; A small test that tests both sbb and adc to ensure it returns data correctly.
; This was noticed in Final Fantasy 7 (steamid 39140) having broken rendering on the title screen.
mov rax, 0x4142434445464748
mov rbx, 0x5152535455565758
mov rcx, 0x6162636465666768

clc
sbb al, bl
sbb ax, bx
sbb eax, ebx
sbb rax, rbx

%assign i 0
%rep 256
sbb al, [rel .data1 + i]
%assign i i+1
%endrep

%assign i 0
%rep 256
sbb ax, [rel .data2 + i]
%assign i i+1
%endrep

%assign i 0
%rep 256
sbb eax, [rel .data4 + i]
%assign i i+1
%endrep


%assign i 0
%rep 256
sbb rax, [rel .data8 + i]
%assign i i+1
%endrep

stc
%assign i 0
%rep 256
sbb al, [rel .data1 + i]
%assign i i+1
%endrep

%assign i 0
%rep 256
sbb ax, [rel .data2 + i]
%assign i i+1
%endrep

%assign i 0
%rep 256
sbb eax, [rel .data4 + i]
%assign i i+1
%endrep


%assign i 0
%rep 256
sbb rax, [rel .data8 + i]
%assign i i+1
%endrep




clc
adc bl, cl
adc bx, cx
adc ebx, ecx
adc rbx, rcx


%assign i 0
%rep 256
adc bl, [rel .data1 + i]
%assign i i+1
%endrep

%assign i 0
%rep 256
adc bx, [rel .data2 + i]
%assign i i+1
%endrep

%assign i 0
%rep 256
adc ebx, [rel .data4 + i]
%assign i i+1
%endrep


%assign i 0
%rep 256
adc rbx, [rel .data8 + i]
%assign i i+1
%endrep


stc
%assign i 0
%rep 256
adc bl, [rel .data1 + i]
%assign i i+1
%endrep

%assign i 0
%rep 256
adc bx, [rel .data2 + i]
%assign i i+1
%endrep

%assign i 0
%rep 256
adc ebx, [rel .data4 + i]
%assign i i+1
%endrep


%assign i 0
%rep 256
adc rbx, [rel .data8 + i]
%assign i i+1
%endrep




hlt

.data1:
%assign i 0
%rep 256
db i
%assign i i+1
%endrep

.data2:
%assign i 0
%rep 256
dw i
%assign i i+1
%endrep

.data4:
%assign i 0
%rep 256
dd i
%assign i i+1
%endrep

.data8:
%assign i 0
%rep 256
dq i
%assign i i+1
%endrep
