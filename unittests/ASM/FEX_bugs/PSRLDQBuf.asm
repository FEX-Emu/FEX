%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0", "0", "0", "0"]
  }
}
%endif

; FEX-Emu had a bug with vpsrldq where if the shift was >= 16 bytes then the top half of the ymm register wasn't modified.
; Adds a simple test to ensure this continues working.
vmovups ymm0, [rel .data]
vpsrldq ymm0, ymm0, 16
hlt

align 32
.data:
dq 0x4142434445464748, 0x5152535455565758, 0x6162636465666768, 0x7172737475767778
