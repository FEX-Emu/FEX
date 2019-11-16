%ifdef CONFIG
{
  "Ignore": [],
  "RegData": {
    "RAX": "1",
    "RBX": "2",
    "RCX": "3",
    "RDX": "4"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

jmp label
label:

mov rsp, 0xe8000000

jmp function
func_return:

lea rbx, [rel function2]
jmp rbx
func2_return:

cmp rcx, rcx
je function3
func3_return:

mov rdx, 4
jne function4
func4_return:

hlt

function:
mov rax, 1
jmp func_return

function2:
mov rbx, 2
jmp func2_return

function3:
mov rcx, 3
jmp func3_return

function4:
mov rdx, 0xDEADBEEF
jmp func4_return

hlt

