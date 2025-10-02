#pragma once
#include <grpcpp/grpcpp.h>
#include <map>
#include <memory>
#include <string>
#include <chrono>

namespace grpc_adapter {

    struct ClientOptions {
        std::string target = "localhost:50051";
        int default_deadline_ms = 3000;

        bool enable_retries = true;
        int  max_retry_attempts = 3;
        int  initial_backoff_ms = 200;
        int  max_backoff_ms = 2000;

        std::multimap<std::string, std::string> default_metadata;
    };

    class Client {
    public:
        explicit Client(ClientOptions opts = {});
        std::shared_ptr<grpc::Channel> channel() const;

        // Try to connect the channel before making calls (optional)
        grpc::Status WaitForReady(std::chrono::milliseconds timeout) const;

        // Default metadata applied to every new ClientContext
        void SetDefaultMetadata(const std::multimap<std::string, std::string>& md);

        // Creates a context with default deadline & metadata
        std::unique_ptr<grpc::ClientContext> MakeContext(
            std::chrono::milliseconds deadline = std::chrono::milliseconds{ 0 }) const;

    private:
        ClientOptions opts_;
        std::shared_ptr<grpc::Channel> channel_;
        std::multimap<std::string, std::string> default_metadata_;
    };

} // namespace grpc_adapter
