%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x10",
    "RBX": "0x10"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rax, 0
mov rcx, 0x10

jmp .head

.top:

add rax, 1
sub rcx, 1

.head:

jrcxz .next
jmp .top
.next:

; Second test
mov rbx, 0
mov rcx, 0xFFFFFFFF00000010
jmp .head2

.top2:
add rbx, 1
sub rcx, 1

.head2:
jecxz .next2
jmp .top2

.next2:

hlt
