/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "extensions/fastfinality/src/dbrb/DbrbProcess.h"

namespace catapult { namespace mocks {
	class MockDbrbProcess : public dbrb::DbrbProcess {
	public:
		MockDbrbProcess(
				std::weak_ptr<net::PacketWriters> pWriters,
				const net::PacketIoPickerContainer& packetIoPickers,
				const ionet::Node& thisNode,
				const crypto::KeyPair& keyPair,
				const std::shared_ptr<thread::IoThreadPool>& pPool,
				dbrb::TransactionSender&& transactionSender,
				const dbrb::DbrbViewFetcher& dbrbViewFetcher,
				chain::TimeSupplier timeSupplier,
				const supplier<Height>& chainHeightSupplier)
			: DbrbProcess(pWriters,
						  packetIoPickers,
						  thisNode,
						  keyPair,
						  pPool,
						  std::move(transactionSender),
						  dbrbViewFetcher,
						  timeSupplier,
						  chainHeightSupplier) {}

		void broadcast(const dbrb::Payload& payload) {
			DbrbProcess::broadcast(payload);
		}

		void processMessage(const dbrb::Message& message) {
			DbrbProcess::processMessage(message);
		}

		void registerPacketHandlers(ionet::ServerPacketHandlers& packetHandlers) {
			DbrbProcess::registerPacketHandlers(packetHandlers);
		}

		void setDeliverCallback(const DeliverCallback& callback) {
			DbrbProcess::setDeliverCallback(callback);
		}

		void updateView(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
			DbrbProcess::updateView(pConfigHolder);
		}

		dbrb::NodeRetreiver& nodeRetreiver() {
			return DbrbProcess::nodeRetreiver();
		}
	};
}}
