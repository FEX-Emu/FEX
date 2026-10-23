%ifdef CONFIG
{
  "RegData": {
    "RAX": "0"
  },
  "Mode": "32BIT"
}
%endif

mov eax, 0x61626364

; FEX-Emu had a bug where fsave+finit wouldn't zero the FIP correctly. Ensure it correctly zeroes.
; First save the x87 state.
fsave [rel .data]

; Now set the FIP inside of the state.
; It's 32-bits by default.
mov [rel .data + 0xc], eax

; Now load the state back.
frstor [rel .data]

; Do an finit, which should clear out the FIP.
finit

; Save the state again and load the FIP to ensure it stuck.
fsave [rel .data2]
mov eax, [rel .data2 + 0xc]
hlt

align 4096
.data:
times 256 db 0x42

.data2:
times 256 db 0x52
