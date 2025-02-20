/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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
#include "BatchPacketReader.h"
#include "ConnectResult.h"
#include "IoTypes.h"
#include "PacketSocketOptions.h"

namespace catapult {
	namespace ionet {
		struct NodeEndpoint;
		struct Packet;
	}
}

namespace catapult { namespace ionet {

	// region SslPacketSocket

	/// Asio socket wrapper that natively supports packets.
	/// This wrapper is threadsafe but does not prevent interleaving reads or writes.
	class SslPacketSocket : public PacketIo, public BatchPacketReader {
	public:
		/// Statistics about a socket.
		struct Stats {
			/// \c true if the socket is open.
			bool IsOpen;

			/// Number of unprocessed bytes.
			size_t NumUnprocessedBytes;
		};

		using StatsCallback = consumer<const Stats&>;

	public:
		~SslPacketSocket() override = default;

	public:
		/// Retrieves statistics about this socket and passes them to \a callback.
		virtual void stats(const StatsCallback& callback) = 0;

		/// Closes the socket.
		virtual void close() = 0;

		/// Aborts the current operation and closes the socket.
		virtual void abort() = 0;

		/// Gets a buffered interface to the packet socket.
		virtual std::shared_ptr<PacketIo> buffered() = 0;
	};

	// endregion

	// region SslPacketSocketInfo

	/// Tuple composed of (resolved) host, public key and packet socket.
	class SslPacketSocketInfo {
	public:
		/// Creates an empty info.
		SslPacketSocketInfo();

		/// Creates an info around \a host, \a publicKey and \a pSslPacketSocket.
		SslPacketSocketInfo(const std::string& host, const Key& publicKey, const std::shared_ptr<SslPacketSocket>& pSslPacketSocket);

	public:
		/// Gets the host.
		const std::string& host() const;

		/// Gets the public key.
		const Key& publicKey() const;

		/// Gets the socket.
		const std::shared_ptr<SslPacketSocket>& socket() const;

	public:
		/// Returns \c true if this info is not empty.
		explicit operator bool() const;

	private:
		std::string m_host;
		Key m_publicKey;
		std::shared_ptr<SslPacketSocket> m_pPacketSocket;
	};

	// endregion

	// region Accept

	/// Callback for an accepted socket.
	using SslAcceptCallback = consumer<const SslPacketSocketInfo&>;

	/// Accepts a connection using \a ioContext and \a acceptor and calls \a accept on completion configuring the socket with \a options.
	void Accept(
			boost::asio::io_context& ioContext,
			boost::asio::ip::tcp::acceptor& acceptor,
			const PacketSocketOptions& options,
			const SslAcceptCallback& accept);

	// endregion

	// region Connect

	/// Callback for a connected socket.
	using SslConnectCallback = consumer<ConnectResult, const SslPacketSocketInfo&>;

	/// Attempts to connect a socket to the specified \a endpoint using \a ioContext and calls \a callback on
	/// completion configuring the socket with \a options. The returned function can be used to cancel the connect.
	/// \note User callbacks passed to the connected socket are serialized.
	action Connect(
			boost::asio::io_context& ioContext,
			const PacketSocketOptions& options,
			const NodeEndpoint& endpoint,
			const SslConnectCallback& callback);

	// endregion
}}
