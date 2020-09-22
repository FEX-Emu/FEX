%ifdef CONFIG
{
  "RegData": {
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

; this just ensures the thunking system runs
; it doesn't actually load a lib

; create struct in ram
lea rdi, [rel libname]
lea rax, [rel init_struct]
mov [rax + 0], rdi
mov [rax + 8], rdi

; thunk
lea rdi, [rel init_struct]
lea rax, [rel thunk_test]
call rax

hlt

thunk_test:
db 0xf, 0x3f
db 'fex:loadlib'
db 0

libname db 'not_a_real_lib', 0
db 0

align 8
init_struct dq 0, 0