%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x61626364"
  },
  "Mode": "32BIT"
}
%endif

mov eax, 0x61626364

; Similar to the fsave_rstore_ip test. By specification fxsave doesn't save x87 pointer registers unless the exception-summary (ES)
; bit is set. This only occurs when unmasked x87 exceptions occur.
; We can test this by using fxsave to save, but then load the data back with frstor.
; AMD documentation claims:
; > FXRSTOR does not restore the x87 error pointers (last instruction pointer, last data pointer, and last
; > Opcode), except when FXRSTOR sets FSW.ES=1 after recomputing it from the error mask bits in
; > FCW and error status bits in FSW.
; This is untrue as fxrstor restores the FIP regardless. This can be seen with an fxsave+fxrstor+fsave dance we do below.

; First save all the state.
fxsave [rel .data]

; Now set the FIP inside of the state.
; It's 32-bits by default and it lives at 0x8 with fxsave instead of 0xc like fsave!
mov [rel .data + 0x8], eax

; Now load the state back.
fxrstor [rel .data]

mov eax, 0x71727374

; Save the state again (this time with fsave!) and load the FIP to ensure it stuck.
fsave [rel .data2]
mov eax, [rel .data2 + 0xc]
hlt

align 4096
.data:
times 512 db 0x42

.data2:
times 512 db 0x52
