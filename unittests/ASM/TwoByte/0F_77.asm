%ifdef CONFIG
{
  "RegData": {
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

emms ; Just ensure it runs

hlt
