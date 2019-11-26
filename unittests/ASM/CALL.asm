%ifdef CONFIG
{
  "Ignore": [],
  "RegData": {
    "RAX": "1",
    "RBX": "2"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

jmp label
label:

mov rsp, 0xe8000000

; Test direct literal call
call function

; Move the absolute address of function2 in to rbx and call it
lea rbx, [rel function2]
call rbx

hlt

function:
mov rax, 1
ret

function2:
mov rbx, 2
ret

hlt
