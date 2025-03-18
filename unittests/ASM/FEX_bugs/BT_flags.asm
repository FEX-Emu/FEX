%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xcafe"
  }
}
%endif

mov ebx, 137
mov rdx, 0xe0000000

%macro case 1
  ; set zero flag
  xor eax, eax

  ; zero flag should still be set after bt
  %1 ebx, 1
  jnz .bad
  %1 dword [rdx], ebx
  jnz .bad

  ; now clear the zero flag
  add eax, 1

  ; zero flag should still be clear after bt
  %1 eax, 1
  jz .bad
  %1 dword [rdx], ebx
  jz .bad
%endmacro

; Repeat for each bitwise op
case bt
case btc
case bts
case btr

.good:
mov rax, 0xcafe
hlt

.bad:
mov rax, 0xdeadbeef
hlt
