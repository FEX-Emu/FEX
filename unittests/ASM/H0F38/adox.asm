%ifdef CONFIG
{
  "RegData": {
      "RAX": "0",
      "RBX": "1",
      "RCX": "0xFFFFFFFFFFFFFFFF",
      "RDX": "0",
      "RSI": "0xFFFFFFFF",
      "RDI": "1"
  },
  "HostFeatures": ["ADX"]
}
%endif

; Test with no overflow
mov rax, -1
mov rbx, 1
adox rax, rbx

; Test with overflow (flag set from previous adox)
mov rbx, 1
mov rcx, -1
adox rbx, rcx

; Clear OF for 32-bit tests.
test al, al

; 32-bit registers

; Test with no overflow
mov edx, -1
mov esi, 1
adox edx, esi

; Test with overflow (flag set from previous adox)
mov edi, 1
mov esi, -1
adox edi, esi

hlt
