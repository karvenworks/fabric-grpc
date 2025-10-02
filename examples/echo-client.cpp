// examples/echo-client.cpp
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>

#include <grpcpp/grpcpp.h>
#include "../include/client.hpp"   // your helper
#include "echo.grpc.pb.h"            // generated from proto

using namespace std::chrono_literals;

static std::atomic<bool> g_running{ true };
static void on_sigint(int) { g_running = false; }

int main() {
    // Handle Ctrl+C to stop the loop
    std::signal(SIGINT, on_sigint);

    // Create client (adjust target if your server runs elsewhere)
    grpc_adapter::Client cli({ .target = "localhost:50051",
                               .default_deadline_ms = 2000,
                               .enable_retries = true,
                               .max_retry_attempts = 3,
                               .initial_backoff_ms = 200,
                               .max_backoff_ms = 2000 });

    // Wait for channel readiness (non-fatal if it times out; we’ll retry later)
    auto ready = cli.WaitForReady(2s);
    if (!ready.ok()) {
        std::cerr << "[warn] Channel not ready yet: " << ready.error_message() << "\n";
    }

    // Reuse one stub for all calls
    auto stub = echo::Echo::NewStub(cli.channel());

    std::cout << "Starting echo loop (every 10s). Press Ctrl+C to stop.\n";
    int counter = 0;

    while (g_running.load()) {
        // Prepare request
        echo::Text req;
        req.set_value("ping #" + std::to_string(counter++));

        echo::Text res;
        auto ctx = cli.MakeContext(2s);  // per-call deadline

        auto status = stub->Say(ctx.get(), req, &res);
        if (status.ok()) {
            std::cout << "[ok] " << res.value() << std::endl;
        }
        else {
            std::cerr << "[rpc error] code=" << status.error_code()
                << " msg=" << status.error_message() << std::endl;

            // If server is down/unreachable, try to re-establish the channel briefly.
            if (status.error_code() == grpc::StatusCode::UNAVAILABLE ||
                status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED) {
                auto s = cli.WaitForReady(2s);
                if (!s.ok()) {
                    std::cerr << "[retry] channel not ready yet.\n";
                }
            }
        }

        // Sleep between sends
        for (int i = 0; i < 100 && g_running.load(); ++i) {
            std::this_thread::sleep_for(100ms); // responsive to Ctrl+C
        }
    }

    std::cout << "Exiting.\n";
    return 0;
}
