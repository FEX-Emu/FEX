%ifdef CONFIG
{
  "RegData": {},
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; FEX had a bug where a mmx->x87 switch would flush the saved NZCV value used for ftst, causing a crash in RA

movq mm0, mm1 ; enters mmx state
ftst ; enters x87 state

hlt
