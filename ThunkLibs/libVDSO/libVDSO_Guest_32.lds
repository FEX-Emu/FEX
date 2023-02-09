SECTIONS {
  . = SIZEOF_HEADERS;
  .hash : { *(.hash) } :text
  .gnu.hash : { *(.gnu.hash) }
  .dynsym : { *(.dynsym) }
  .dynstr : { *(.dynstr) }
  .gnu.version : { *(.gnu.version) }
  .gnu.version_d : { *(.gnu.version_d) }
  .gnu.version_r : { *(.gnu.version_r) }
  .dynamic : { *(.dynamic) } :text :dynamic
  .rodata : {
    *(.rodata*)
      *(.data*)
      *(.sdata*)
      *(.got.plt) *(.got)
      *(.gnu.linkonce.d.*)
      *(.bss*)
      *(.dynbss*)
      *(.gnu.linkonce.b.*)
  } :text

  /DISCARD/ : {
    *(.note)
    *(.note.gnu.property)
    *(.eh_frame_hdr)
    *(.eh_frame)
    *(.symtab)
  }
}

PHDRS {
  text PT_LOAD FLAGS(4 | 1) FILEHDR PHDRS;
  dynamic PT_DYNAMIC FLAGS(4);
  note PT_NOTE FLAGS(4);
}

VERSION {
  LINUX_2.6 {
  global:
    __vdso_time;
    time;
    __vdso_gettimeofday;
    gettimeofday;
    __vdso_clock_gettime;
    clock_gettime;
    __vdso_clock_getres;
    clock_getres;
    __vdso_getcpu;
    getcpu;
    __vdso_clock_gettime64;
    clock_gettime64;
  local: *;
  };
  LINUX_2.5 {
  global:
    __kernel_vsyscall;
    __kernel_sigreturn;
    __kernel_rt_sigreturn;
  local: *;
  };
}
