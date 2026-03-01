#ifdef FORWARD_DECLARE
  namespace UnwinderFuncs {
    int find_proc_info(unw_addr_space_t as,
        unw_word_t ip, unw_proc_info_t *pip,
        int need_unwind_info, void *arg);
    void put_unwind_info(unw_addr_space_t as,
        unw_proc_info_t *pip, void *arg);
    int get_dyn_info_list_addr(unw_addr_space_t as,
        unw_word_t *dilap, void *arg);
    int access_mem(unw_addr_space_t as,
        unw_word_t addr, unw_word_t *valp,
        int write, void *arg);
    int access_reg(unw_addr_space_t as,
        unw_regnum_t regnum, unw_word_t *valp,
        int write, void *arg);
    int access_fpreg(unw_addr_space_t as,
        unw_regnum_t regnum, unw_fpreg_t *fpvalp,
        int write, void *arg);
    int resume(unw_addr_space_t as,
        unw_cursor_t *cp, void *arg);
    int get_proc_name(unw_addr_space_t as,
        unw_word_t addr, char *bufp,
        size_t buf_len, unw_word_t *offp,
        void *arg);
  }
#undef FORWARD_DECLARE
#endif

#ifdef ACCESSOR_IMPL
namespace UnwinderFuncs {
int find_proc_info(unw_addr_space_t as,
    unw_word_t ip, unw_proc_info_t *pip,
    int need_unwind_info, void *arg) {
  // Just claim we don't have anything.
  return -UNW_ENOINFO;
}
void put_unwind_info(unw_addr_space_t as,
    unw_proc_info_t *pip, void *arg) {
  // Unused.
  LogMan::Msg::EFmt("[CoreDumpService] Can't put_unwind_info\n");
}
int get_dyn_info_list_addr(unw_addr_space_t as,
    unw_word_t *dilap, void *arg) {
  // Say we don't have one.
  *dilap = 0;
  return 0;
}
int access_mem(unw_addr_space_t as,
    unw_word_t addr, unw_word_t *valp,
    int write, void *arg) {
  UNWINDER_TYPE *Unwind = (UNWINDER_TYPE*)arg;

  if (write == 0) {
    auto res = Unwind->TryPeekMem(addr, 8);
    if (res) {
      *valp = *res;
    } else {
      // Guest memory was inaccessible. Claim as such so it doesn't retry.
      return -UNW_EUNSPEC;
    }
  }
  else {
    LogMan::Msg::EFmt("[CoreDumpService] Can't write to memory with access_mem\n");
  }
  return 0;
}
int access_reg(unw_addr_space_t as,
    unw_regnum_t regnum, unw_word_t *valp,
    int write, void *arg) {
  UNWINDER_TYPE *Unwind = (UNWINDER_TYPE*)arg;

  if (write == 0) {
    *valp = Unwind->GetReg(regnum);
  }
  else {
    Unwind->SetReg(regnum, *valp);
  }
  return 0;
}
int access_fpreg(unw_addr_space_t as,
    unw_regnum_t regnum, unw_fpreg_t *fpvalp,
    int write, void *arg) {
  // Unused.
  LogMan::Msg::EFmt("[CoreDumpService] Can't fpreg\n");
  return 0;
}
int resume(unw_addr_space_t as,
    unw_cursor_t *cp, void *arg) {
  // Unused.
  LogMan::Msg::EFmt("[CoreDumpService] Can't resume\n");
  return 0;
}
int get_proc_name(unw_addr_space_t as,
    unw_word_t addr, char *bufp,
    size_t buf_len, unw_word_t *offp,
    void *arg) {
  UNWINDER_TYPE *Unwind = (UNWINDER_TYPE*)arg;

  *offp = 0;
  auto Mapping = Unwind->GetFileMapping(addr);
  if (!Mapping) {
    LogMan::Msg::DFmt("No mapping for 0x{:x}", addr);
    return -UNW_EUNSPEC;
  }

  if (!Mapping->ELFMapping) {
    int FD = open(Mapping->Path.c_str(), O_RDONLY);
    Mapping->ELFMapping = ELFMapping::LoadELFMapping(Mapping, FD);
    close(FD);
  }

  if (Mapping->ELFMapping) {
    auto Symbol = ELFMapping::GetSymbolFromAddress(Mapping->ELFMapping, addr);
    if (Symbol) {
      strncpy(bufp, Symbol->Name, buf_len);
      *offp = addr - Symbol->Address;
      return 0;
    }
  }

  if (Mapping->Path.empty()) {
    strncpy(bufp, "GuestJIT", buf_len);
  }
  else {
    strncpy(bufp, std::filesystem::path(Mapping->Path).filename().c_str(), buf_len);
  }

  *offp = (addr - Mapping->Begin);
  return 0;
}
}
#endif
