#pragma once

namespace FEX::RootFS {
  bool Setup(char **const envp);
  void Shutdown();
}
