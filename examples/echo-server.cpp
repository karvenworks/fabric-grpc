#include <grpcpp/grpcpp.h>
#include "../include/server.hpp"
#include "echo.grpc.pb.h"

using grpc::ServerContext;
using grpc::Status;

class EchoImpl final : public echo::Echo::Service {
public:
    Status Say(ServerContext* ctx, const echo::Text* req, echo::Text* res) override {
        (void)ctx;
        res->set_value(req->value());
        return Status::OK;
    }
};

int main() {
    grpc_adapter::ServerOptions opts;
    opts.address = "0.0.0.0:50051";
    opts.enable_reflection = true;
    opts.enable_health = true;

    EchoImpl svc;
    grpc_adapter::Server server(opts);
    server.AddService(&svc);

    if (!server.Start()) {
        std::cerr << "Failed to start server\n";
        return 1;
    }
    std::cout << "Server listening on " << opts.address << std::endl;
    server.Wait();
    return 0;
}
