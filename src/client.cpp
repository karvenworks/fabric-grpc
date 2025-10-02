// src/client.cpp
#include "client.hpp"

#include <grpcpp/grpcpp.h>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace grpc_adapter {

    // helper: seconds (with fractional) like "0.2s" for service config
    static std::string secs_str(double secs) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3) << secs << "s";
        return oss.str();
    }

    Client::Client(ClientOptions opts)
        : opts_(std::move(opts)),
        default_metadata_(opts_.default_metadata) {

        grpc::ChannelArguments args;

        // Optional: client-side retries via service config JSON (applies to all methods)
        if (opts_.enable_retries && opts_.max_retry_attempts > 1) {
            const double init_s = std::max(0.001, opts_.initial_backoff_ms / 1000.0);
            const double max_s = std::max(init_s, opts_.max_backoff_ms / 1000.0);

            const std::string service_config = std::string(R"({
      "methodConfig": [{
        "name": [{}],
        "retryPolicy": {
          "maxAttempts": )") +
                std::to_string(opts_.max_retry_attempts) + R"(,
          "initialBackoff": ")" + secs_str(init_s) + R"(",
          "maxBackoff": ")" + secs_str(max_s) + R"(",
          "backoffMultiplier": 2.0,
          "retryableStatusCodes": ["UNAVAILABLE","DEADLINE_EXCEEDED"]
        }
      }]
    })";

            args.SetServiceConfigJSON(service_config);
        }

        // Nice-to-haves: keepalive defaults (tweak later if needed)
        args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 20'000);
        args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 10'000);
        args.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);

        // Create channel
        channel_ = grpc::CreateCustomChannel(
            opts_.target.empty() ? "localhost:50051" : opts_.target,
            grpc::InsecureChannelCredentials(),  // swap for SSL creds later
            args);
    }

    std::shared_ptr<grpc::Channel> Client::channel() const {
        return channel_;
    }

    grpc::Status Client::WaitForReady(std::chrono::milliseconds timeout) const {
        const auto dl = std::chrono::system_clock::now() + timeout;
        if (channel_->WaitForConnected(dl)) {
            return grpc::Status::OK;
        }
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "channel not connected before deadline");
    }

    void Client::SetDefaultMetadata(const std::multimap<std::string, std::string>& md) {
        default_metadata_ = md;
    }

    std::unique_ptr<grpc::ClientContext> Client::MakeContext(std::chrono::milliseconds deadline) const {
        auto ctx = std::make_unique<grpc::ClientContext>();
        // Deadline
        if (deadline.count() <= 0) {
            deadline = std::chrono::milliseconds(opts_.default_deadline_ms);
        }
        ctx->set_deadline(std::chrono::system_clock::now() + deadline);

        // Default metadata
        for (const auto& kv : default_metadata_) {
            ctx->AddMetadata(kv.first, kv.second);
        }
        return ctx;
    }

} // namespace grpc_adapter
