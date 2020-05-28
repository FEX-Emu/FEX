%ifdef CONFIG
{
}
%endif

mov rdx, 0xe0000000
; Just to ensure execution
fldcw [rdx]

hlt
