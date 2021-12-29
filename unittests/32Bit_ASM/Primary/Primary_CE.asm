%ifdef CONFIG
{
  "RegData": {
  },
  "Mode": "32BIT"
}
%endif

; Clear OF just incase
test eax, eax

; Just ensure it executes safely
into

hlt
