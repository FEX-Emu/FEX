%ifdef CONFIG
{
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rdx, 0xe0000000
; Just to ensure execution
fldcw [rdx]

hlt
