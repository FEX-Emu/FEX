%ifdef CONFIG
{
  "RegData": {
    "RAX": "1",
    "RBX": "0xe0000fe0",
    "RSP": "0xe0000ff0"
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
o64 call far [rsp]

hlt

.success:
mov rax, 1
mov rbx, rsp
o64 retf
