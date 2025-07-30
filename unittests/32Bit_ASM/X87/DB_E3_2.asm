%ifdef CONFIG
{
  "RegData": {
    "RAX": "0"
  },
  "Mode": "32BIT"
}
%endif

; Tests that fninit clears the status word (which includes the IE flag)
fninit
fldz
fldz
fdiv ; sets IE flag

fninit
fnstsw ax

hlt
