%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4",
    "RBX": "0xFFFFFFFFFFFFFFF4",
    "RCX": "0x0",
    "RDX": "0x1337"
  },
  "HostFeatures": ["BMI2"]
}
%endif

; FEX had a bug where bzhi would fail to update SF. Test that bzhi correctly
; sets ZF/SF correctly based on the result.

mov rcx, 4
mov rbx, -12

; Result is 0x4
bzhi rax, rbx, rcx
mov rdx, 0xdead1
jz .fail
mov rdx, 0xdead2
js .fail

; Result is -12
mov rcx, 64
bzhi rdx, rbx, rcx
mov rdx, 0xdead3
jz .fail
mov rdx, 0xdead4
jns .fail

; Result is 0x00
mov rdx, 0
bzhi rcx, rbx, rdx
mov rdx, 0xdead5
jnz .fail
mov rdx, 0xdead6
js .fail

mov rdx, 0x1337
hlt

.fail:
hlt
