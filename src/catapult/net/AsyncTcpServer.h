/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "ConnectionSettings.h"
#include "catapult/ionet/PacketSocket.h"
#include "catapult/ionet/SslPacketSocket.h"

namespace catapult { namespace thread { class IoThreadPool; } }

namespace catapult { namespace net {

	using AcceptHandler = consumer<const ionet::AcceptedPacketSocketInfo&>;
	using SslAcceptHandler = consumer<const ionet::SslPacketSocketInfo&>;

	using ConfigureSocketHandler = consumer<ionet::NetworkSocket&>;

	/// Settings used to configure AsyncTcpServer behavior.
	template<typename TAcceptHandler>
	struct AsyncTcpServerSettingsT {
	public:
		/// Creates a structure with a preconfigured accept handler (\a accept).
		explicit AsyncTcpServerSettingsT(const TAcceptHandler& accept)
			: Accept(accept)
			, ConfigureSocket([](const auto&) {})
			, PacketSocketOptions(ConnectionSettings().toSocketOptions())
		{}

	public:
		/// Accept handler (must be set via constructor).
		const TAcceptHandler Accept;

		// The configure socket handler.
		ConfigureSocketHandler ConfigureSocket;

		/// Packet socket options.
		ionet::PacketSocketOptions PacketSocketOptions;

		// The maximum number of pending connections (backlog size).
		uint16_t MaxPendingConnections = 100;

		// The maximum number of active connections.
		uint32_t MaxActiveConnections = 25;

		/// \c true if the server should reuse ports already in use.
		bool AllowAddressReuse = false;
	};

	using AsyncTcpServerSettings = AsyncTcpServerSettingsT<AcceptHandler>;
	using AsyncSslTcpServerSettings = AsyncTcpServerSettingsT<SslAcceptHandler>;

	/// An async TCP server.
	class AsyncTcpServer {
	public:
		virtual ~AsyncTcpServer() = default;

	public:
		/// Number of asynchronously started (but not completed) socket accepts.
		virtual uint32_t numPendingAccepts() const = 0;

		/// Current number of active connections.
		virtual uint32_t numCurrentConnections() const = 0;

		/// Total number of connections during the server's lifetime.
		virtual uint32_t numLifetimeConnections() const = 0;

	public:
		/// Shuts down the server.
		virtual void shutdown() = 0;
	};

	/// Creates an async tcp server listening on \a endpoint with the specified \a settings using the specified
	/// thread pool (\a pPool).
	std::shared_ptr<AsyncTcpServer> CreateAsyncTcpServer(
			const std::shared_ptr<thread::IoThreadPool>& pPool,
			const boost::asio::ip::tcp::endpoint& endpoint,
			const AsyncTcpServerSettings& settings);

	/// Creates an async tcp server listening on \a endpoint with the specified \a settings using the specified
	/// thread pool (\a pPool).
	std::shared_ptr<AsyncTcpServer> CreateAsyncSslTcpServer(
			const std::shared_ptr<thread::IoThreadPool>& pPool,
			const boost::asio::ip::tcp::endpoint& endpoint,
			const AsyncSslTcpServerSettings& settings);
}}
