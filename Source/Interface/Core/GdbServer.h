
#include <thread>

#include "Interface/Context/Context.h"

#include "Common/NetStream.h"

namespace FEXCore {

class GdbServer {
public:
    GdbServer(FEXCore::Context::Context *ctx, FEXCore::CodeLoader *Loader);
    void StartAndBlock() { StartThread(OpenSocket()); }

private:
    std::unique_ptr<std::iostream> OpenSocket();
    void StartThread(std::unique_ptr<std::iostream> stream);
    void GdbServerLoop(std::unique_ptr<std::iostream> stream);
    std::string ReadPacket(std::iostream &stream);
    void SendPacket(std::ostream &stream, std::string packet);

    std::string ProcessPacket(std::string &packet);
    std::string handleQuery(std::string &packet);
    std::string handleXfer(std::string &packet);
    std::string handleMemory(std::string &packet);
    std::string handleV(std::string& packet);

    std::string readRegs();

    FEXCore::Context::Context *CTX;
    std::thread gdbServerThread;
    uint64_t data_offset;
};

}



