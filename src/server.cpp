#include "server.hpp"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>

namespace grpc_adapter {

    Server::Server(ServerOptions opts) : opts_(std::move(opts)) {}

    void Server::AddService(grpc::Service* service) {
        services_.push_back(service);
    }

    bool Server::Start() {
        grpc::ServerBuilder builder;

        if (opts_.enable_health) {
            grpc::EnableDefaultHealthCheckService(true);
        }
        if (opts_.enable_reflection) {
            grpc::reflection::InitProtoReflectionServerBuilderPlugin();
        }
        if (opts_.max_receive_message_size > 0) {
            builder.SetMaxReceiveMessageSize(opts_.max_receive_message_size);
        }
        if (opts_.max_send_message_size > 0) {
            builder.SetMaxSendMessageSize(opts_.max_send_message_size);
        }

        auto creds = opts_.credentials ? opts_.credentials
            : grpc::InsecureServerCredentials();

        builder.AddListeningPort(opts_.address, creds);
        for (auto* svc : services_) {
            builder.RegisterService(svc);
        }

        server_ = builder.BuildAndStart();
        return static_cast<bool>(server_);
    }

    void Server::Wait() {
        if (server_) server_->Wait();
    }

    void Server::Shutdown() {
        if (server_) server_->Shutdown();
    }

    bool Server::IsRunning() const {
        return static_cast<bool>(server_);
    }

} // namespace grpc_adapter
