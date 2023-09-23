/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MockDbrbViewFetcher.h"
#include "extensions/fastfinality/src/dbrb/DbrbProcess.h"
#include "plugins/txes/dbrb/src/cache/DbrbViewFetcherImpl.h"
#include "src/catapult/crypto/KeyPair.h"
#include "src/catapult/dbrb/DbrbDefinitions.h"
#include "src/catapult/dbrb/Messages.h"
#include "src/catapult/net/PacketIoPickerContainer.h"
#include "tests/test/core/ThreadPoolTestUtils.h"
#include "tests/test/net/mocks/MockPacketWriters.h"

namespace catapult { namespace mocks {

	class MockDbrbProcess : public dbrb::DbrbProcess {
	public:
		using DisseminationHistory = std::vector<std::pair<const std::shared_ptr<dbrb::Message>&, std::set<dbrb::ProcessId>>>;

	public:
		explicit MockDbrbProcess(
				const dbrb::ProcessId& processId,
				bool fakeDissemination = false,
				const dbrb::DbrbConfiguration& dbrbConfig = dbrb::DbrbConfiguration::Uninitialized(),
				std::weak_ptr<net::PacketWriters> pWriters = std::weak_ptr<mocks::MockPacketWriters>(),
				const net::PacketIoPickerContainer& packetIoPickers = {},
				const crypto::KeyPair& keyPair = crypto::KeyPair::FromPrivate({}),
				const config::ImmutableConfiguration& immutableConfig = config::ImmutableConfiguration::Uninitialized(),
				const std::shared_ptr<thread::IoThreadPool>& pPool = test::CreateStartedIoThreadPool(1),
				const dbrb::DbrbViewFetcher& dbrbViewFetcher = MockDbrbViewFetcher(),
				chain::TimeSupplier timeSupplier = []() { return Timestamp(); },
				const supplier<Height>& chainHeightSupplier = []() { return Height(); });

		void setCurrentView(const dbrb::View& view);

		void broadcast(const dbrb::Payload& payload) override;
		void processMessage(const dbrb::Message& message) override;
		Signature sign(const dbrb::Payload& payload);

		void disseminate(const std::shared_ptr<dbrb::Message>& pMessage, std::set<dbrb::ProcessId> recipients) override;
		void send(const std::shared_ptr<dbrb::Message>& pMessage, const dbrb::ProcessId& recipient) override;

		void onAcknowledgedMessageReceived(const dbrb::AcknowledgedMessage& message) override;
		void onAcknowledgedQuorumCollected(const dbrb::AcknowledgedMessage& message);
		void onCommitMessageReceived(const dbrb::CommitMessage& message) override;

		const std::set<Hash256>& deliveredPayloads();
		const dbrb::ProcessId& id();
		const dbrb::View& currentView();
		std::map<Hash256, dbrb::BroadcastData>& broadcastData();
		const DisseminationHistory& disseminationHistory();
		dbrb::QuorumManager& getQuorumManager(const Hash256& payloadHash);

	public:
		inline static std::vector<std::shared_ptr<MockDbrbProcess>> DbrbProcessPool;

	private:
		/// Set of payload hashes delivered by this process.
		std::set<Hash256> m_deliveredPayloads;

		/// If true, each invocation of message dissemination doesn't call recipients' callback methods.
		/// Regardless of this value, all attempts to disseminate messages are recorded in \c m_disseminationHistory.
		bool m_fakeDissemination;

		/// Recorded history of all process' attempts to disseminate messages.
		/// Each entry contains a pointer to disseminated message and a set of all recipients of that message.
		DisseminationHistory m_disseminationHistory;
	};
}}
