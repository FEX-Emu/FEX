%ifdef CONFIG
{
  "RegData": {
    "RAX": "1",
    "RBX": "0xe0000fe8",
    "RSP": "0xe0002224"
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
call far dword [esp]

hlt

.success:
mov rax, 1
mov rbx, rsp
o32 retf 0x1234
