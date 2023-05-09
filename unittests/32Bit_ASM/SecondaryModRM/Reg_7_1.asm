%ifdef CONFIG
{
  "Mode": "32BIT",
  "HostFeatures": ["Linux"]
}
%endif

; We can't really check the results of this
rdtscp

hlt
