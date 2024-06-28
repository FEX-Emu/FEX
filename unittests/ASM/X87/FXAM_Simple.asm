;; Simpler versions of FXAM_Push* tests.
;; In hostrunner tests this will fail because we mentioned below there's no support
;; for the zero flag. In hostrunner RCX should contain 0x4000 instead of 0x400.
%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x6",
    "RBX": "0x0400",
    "RCX": "0x0400",
    "RDX": "0x4100"
  }
}
%endif

mov rdx, 0xe0000000

fninit
;; Before adding anything to the stack, lets examine it.
;; The result should be empty.
fxam
fwait

fnstsw ax 
and ax, 0x4500 ; should be 0x4100 for zero
mov edx, eax

fldz
fxam 
fwait 

fnstsw ax
and ax, 0x4500 ; should be 0x4000 for zero, but there's no support for it at the moment, so it'll return 0x0400 as it does for a normal number.
mov ecx, eax

fld1
fxam
fwait

fnstsw ax
mov ebx, eax
and ebx, 0x4500 ; should be 0x0400 for normal

;; Top should be 6
;; right shift status word by 11 and and with 0x7.
shr eax, 11
and eax, 0x7


hlt
