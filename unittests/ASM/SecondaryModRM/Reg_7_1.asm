%ifdef CONFIG
{
  "HostFeatures": ["Linux"]
}
%endif

; We can't really check the results of this
; Just ensure we execute it
rdtscp

hlt
