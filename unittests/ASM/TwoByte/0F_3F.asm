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

lea rdi, [rel libname]
lea rax, [rel thunk_test]
call rax

hlt

thunk_test:
db 0xf, 0x3f
db 'fex:loadlib'
db 0

libname db 'not_a_real_lib'
db 0