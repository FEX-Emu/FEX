%ifdef CONFIG
{
}
%endif

mov rdx, 0xe0000000
clflush [rdx]
hlt
