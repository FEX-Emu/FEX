%ifdef CONFIG
{
  "Mode": "32BIT"
}
%endif

; We can't really check the results of this
rdtscp

hlt
