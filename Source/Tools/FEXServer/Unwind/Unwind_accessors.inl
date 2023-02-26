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
      fmt::print("[CoreDumpService] Can't put_unwind_info\n");
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
      if (!Unwind->CoreDump->AddressHasAccess(addr, write != 0)) {
        // If the guest doesn't have access, then don't try reading.
        return -UNW_EUNSPEC;
      }

      if (write == 0) {
        *valp = Unwind->PeekMem(addr, 8);
      }
      else {
        fmt::print("[CoreDumpService] Can't write to memory with access_mem\n");
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
      fmt::print("[CoreDumpService] Can't fpreg\n");
      return 0;
    }
    int resume(unw_addr_space_t as,
        unw_cursor_t *cp, void *arg) {
      // Unused.
      fmt::print("[CoreDumpService] Can't resume\n");
      return 0;
    }
    int get_proc_name(unw_addr_space_t as,
        unw_word_t addr, char *bufp,
        size_t buf_len, unw_word_t *offp,
        void *arg) {
      UNWINDER_TYPE *Unwind = (UNWINDER_TYPE*)arg;

      auto Mapping = Unwind->CoreDump->GetFileMapping(addr);
      if (!Mapping) {
        return -UNW_EUNSPEC;
      }

      if (!Mapping->ELFMapping) {
        Mapping->ELFMapping.reset(ELFMapping::LoadELFMapping(Mapping, Unwind->GetFD(&Mapping->Path)));
      }

      if (Mapping->ELFMapping) {
        auto Symbol = ELFMapping::GetSymbolFromAddress(Mapping->ELFMapping.get(), addr);
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

  void UNWINDER_TYPE::Backtrace() {
    if (!CanBacktrace) {
      fmt::print(stderr, "[CoreDumpService] Couldn't initialize backtracer\n");
      return;
    }

    BacktraceHeader();

    int ret;
    int Frame{};
    char name[256];

    auto Desc = CoreDump->GetDescription();
    fmt::print(stderr, "                Stack trace of thread {}:\n", Desc.tid);

    do
    {
      unw_word_t ip, sp, off;
      unw_get_reg (&cursor, UNW_REG_IP, &ip);
      unw_get_reg (&cursor, UNW_REG_SP, &sp);
      bool NoProc = false;
      std::string Buffer{};
      if (unw_get_proc_name (&cursor, name, sizeof (name), &off) == 0)
      {
        if (off)
          Buffer = fmt::format("<{} + 0x{:x}>", name, (long) off);
        else
          Buffer = fmt::format("<{}>", name);
      }
      else {
        NoProc = true;
      }

      fmt::print(stderr,
        sizeof(unw_word_t) == 4 ?
        "                #{}  {}0x{:08x} {:<32} sp=(0x{:08x})\n" :
        "                #{}  {}0x{:016x} {:<32} sp=(0x{:016x})\n"
        , Frame, NoProc ? "NoELFParse: " : "", (long) ip, Buffer, (long) sp);

      ret = unw_step (&cursor);

      ++Frame;
      if (ret < 0)
      {
        unw_get_reg (&cursor, UNW_REG_IP, &ip);
        fmt::print(stderr, "FAILURE: unw_step() returned {} for ip: 0x{:x}\n", ret, (long) ip);
        return;
      }
    }
    while (ret > 0);
  }

#undef ACCESSOR_IMPL
#undef UNWINDER_TYPE

#endif
