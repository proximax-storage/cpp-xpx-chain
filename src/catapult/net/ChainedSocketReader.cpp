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

#include "ChainedSocketReader.h"
#include "catapult/ionet/PacketSocket.h"
#include "catapult/ionet/SslPacketSocket.h"
#include "catapult/ionet/SocketReader.h"

namespace catapult { namespace net {

	namespace {
		template<typename TPacketSocket>
		class DefaultChainedSocketReader
				: public ChainedSocketReader
				, public std::enable_shared_from_this<DefaultChainedSocketReader<TPacketSocket>> {
		public:
			DefaultChainedSocketReader(
					const std::shared_ptr<TPacketSocket>& pPacketSocket,
					const std::shared_ptr<ionet::PacketIo>& pBufferedIo,
					const ionet::ServerPacketHandlers& serverHandlers,
					const ionet::ReaderIdentity& identity,
					const ChainedSocketReader::CompletionHandler& completionHandler)
					: m_pPacketSocket(pPacketSocket)
					, m_identity(identity)
					, m_completionHandler(completionHandler)
					, m_pReader(CreateSocketReader(m_pPacketSocket, pBufferedIo, serverHandlers, identity))
			{}

		public:
			void start() override {
				m_pReader->read([pThis = this->shared_from_this()](auto code) { pThis->read(code); });
			}

			void stop() override {
				m_pPacketSocket->close();
			}

		private:
			void read(ionet::SocketOperationCode code) {
				switch (code) {
				case ionet::SocketOperationCode::Success:
					return;

				case ionet::SocketOperationCode::Insufficient_Data:
					// Insufficient_Data signals the definitive end of a (successful) batch operation,
					// whereas Success can be returned multiple times
					return start();

				default:
					CATAPULT_LOG(warning) << m_identity << " read completed with error: " << code;
					return m_completionHandler(code);
				}
			}

		private:
			std::shared_ptr<TPacketSocket> m_pPacketSocket;
			ionet::ReaderIdentity m_identity;
			ChainedSocketReader::CompletionHandler m_completionHandler;
			std::unique_ptr<ionet::SocketReader> m_pReader;
		};
	}

	std::shared_ptr<ChainedSocketReader> CreateChainedSocketReader(
			const std::shared_ptr<ionet::PacketSocket>& pPacketSocket,
			const std::shared_ptr<ionet::PacketIo>& pBufferedIo,
			const ionet::ServerPacketHandlers& serverHandlers,
			const ionet::ReaderIdentity& identity,
			const ChainedSocketReader::CompletionHandler& completionHandler) {
		return std::make_shared<DefaultChainedSocketReader<ionet::PacketSocket>>(pPacketSocket, pBufferedIo, serverHandlers, identity, completionHandler);
	}

	std::shared_ptr<ChainedSocketReader> CreateChainedSocketReader(
			const std::shared_ptr<ionet::SslPacketSocket>& pPacketSocket,
			const std::shared_ptr<ionet::PacketIo>& pBufferedIo,
			const ionet::ServerPacketHandlers& serverHandlers,
			const ionet::ReaderIdentity& identity,
			const ChainedSocketReader::CompletionHandler& completionHandler) {
		return std::make_shared<DefaultChainedSocketReader<ionet::SslPacketSocket>>(pPacketSocket, pBufferedIo, serverHandlers, identity, completionHandler);
	}
}}
