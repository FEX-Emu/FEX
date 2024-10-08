%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFFFFFFFFFFFFF0"
  }
}
%endif

mov al, 0xF0
cbw
cwde
cdqe

hlt
