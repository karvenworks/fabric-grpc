#pragma once
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <vector>

namespace grpc_adapter {

	struct ServerOptions {
		std::string address = "0.0.0.0:50051";
		bool enable_reflection = true;
		bool enable_health = true;
		// Set >0 to override defaults; <=0 leaves gRPC defaults
		int max_receive_message_size = -1;
		int max_send_message_size = -1;

		// If null, uses InsecureServerCredentials(); swap for SSL later.
		std::shared_ptr<grpc::ServerCredentials> credentials;
	};

	class Server {
	public:
		explicit Server(ServerOptions opts = {});
		// Register services before Start(); you can call multiple times.
		void AddService(grpc::Service* service);

		// Build and start the server; returns true on success.
		bool Start();

		// Block until shutdown.
		void Wait();

		// Graceful shutdown (non-blocking).
		void Shutdown();

		bool IsRunning() const;

	private:
		ServerOptions opts_;
		std::vector<grpc::Service*> services_;
		std::unique_ptr<grpc::Server> server_;
	};

} // namespace grpc_adapter
