%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xaaaa",
    "RBX": "0xaaaa",
    "RSI": "0xbbbb"
  }
}
%endif

; FEX had a bug with mov+xchg back-to-back due to failing to account for a copy
; inserted during RA. This resulted in a hang starting the game Hades due to the
; mov+xchg code sequence found within Wine's x64 build of ucrtbase.dll.

mov rax, 0xaaaa
mov rbx, 0xbbbb
mov rsi, 0xcccc

; step 1
mov    rsi,rax
; rax = 0xaaaa
; rbx = 0xbbbb
; rsi = 0xaaaa

; step 2
xchg   rbx,rsi
; rax = 0xaaaa
; rbx = 0xaaaa
; rsi = 0xbbbb

hlt
