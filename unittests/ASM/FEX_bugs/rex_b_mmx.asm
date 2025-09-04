%ifdef CONFIG
{
  "HostFeatures": ["SSSE3"]
}
%endif

; FEX had a bug where the REX.B prefix would cause out of bounds MMX register access, when real HW ignores its presence

db 0x41 ; REX.B
psignd mm4, mm0

hlt
