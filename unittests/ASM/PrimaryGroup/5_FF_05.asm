%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

mov rsp, 0xe000_1000
mov ax, cs
lea edi, [rel .success]

sub rsp, 16
mov [rsp], edi
mov [rsp+4], cs

mov rax, 0
jmp far [rsp]

hlt

.success:
mov rax, 1
hlt
