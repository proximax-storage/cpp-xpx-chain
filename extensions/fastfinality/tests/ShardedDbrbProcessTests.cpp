/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "fastfinality/src/dbrb/ShardedDbrbProcess.h"
#include "catapult/dbrb/DbrbDefinitions.h"
#include "catapult/dbrb/Messages.h"
#include "catapult/ionet/NodePacketIoPair.h"
#include "catapult/thread/IoThreadPool.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/core/ThreadPoolTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"
#include "catapult/utils/NetworkTime.h"
#include <boost/asio.hpp>

namespace catapult { namespace fastfinality {

#define TEST_CLASS ShardedDbrbProcessTests

	namespace {
		class MockMessageSender : public dbrb::MessageSender, public std::enable_shared_from_this<MockMessageSender> {
		public:
			explicit MockMessageSender(
				const std::shared_ptr<thread::IoThreadPool>& pPool,
				std::map<dbrb::ProcessId, std::shared_ptr<dbrb::ShardedDbrbProcess>>& processes,
				dbrb::ViewData unreachableNodes)
					: m_strand(pPool->ioContext())
					, m_unreachableNodes(std::move(unreachableNodes))
			{}

			~MockMessageSender() override = default;

		public:
			void enqueue(const dbrb::Payload& payload, bool dropOnFailure, const dbrb::ViewData& recipients) override {
				auto pMessage = m_converter.toMessage(*payload);
				boost::asio::post(m_strand, [pThisWeak = weak_from_this(), pMessage, recipients]() {
					auto pThis = pThisWeak.lock();
					if (pThis) {
						for (auto& id : recipients) {
							if (pThis->m_unreachableNodes.find(id) == pThis->m_unreachableNodes.cend()) {
								auto pProcess = pThis->m_processes.at(id).lock();
								if (pProcess)
									pProcess->processMessage(*pMessage);
							}
						}
					}
				});
			}

			void clearQueue() override {}
			void findNodes(dbrb::ViewData requestedIds) override {}
			void addNodes(const std::vector<ionet::Node>& nodes) override {}
			void sendNodes(const std::vector<ionet::Node>& nodes, const dbrb::ProcessId& recipient) override {}
			void removeNode(const dbrb::ProcessId& id) override {}

			dbrb::ViewData getUnreachableNodes(dbrb::ViewData& view) const override {
				for (const auto& id : m_unreachableNodes)
					view.erase(id);

				return m_unreachableNodes;
			}

			std::vector<ionet::Node> getKnownNodes(dbrb::ViewData& view) const {
				return {};
			}

			size_t getUnreachableNodeCount(const dbrb::ViewData& view) const override {
				return m_unreachableNodes.size();
			}

			void addProcess(const std::shared_ptr<dbrb::ShardedDbrbProcess>& pProcess) {
				m_processes.emplace(pProcess->id(), pProcess);
			}

		private:
			dbrb::NetworkPacketConverter m_converter;
			boost::asio::io_context::strand m_strand;
			std::map<dbrb::ProcessId, std::weak_ptr<dbrb::ShardedDbrbProcess>> m_processes;
			dbrb::ViewData m_unreachableNodes;
		};

		std::vector<crypto::KeyPair> CreateKeyPairs(size_t count) {
			std::vector<crypto::KeyPair> keyPairs;
			for (auto i = 0u; i < count; ++i)
				keyPairs.push_back(crypto::KeyPair::FromPrivate(test::GenerateRandomPrivateKey()));

			return keyPairs;
		}

		class MockDbrbViewFetcher : public dbrb::DbrbViewFetcher {
		public:
			explicit MockDbrbViewFetcher(dbrb::ViewData view)
				: m_view(std::move(view))
			{}

		public:
			dbrb::ViewData getView(Timestamp timestamp) const override {
				return m_view;
			}
			
			Timestamp getExpirationTime(const dbrb::ProcessId& processId) const override {
				return Timestamp();
			}

			BlockDuration getBanPeriod(const dbrb::ProcessId& processId) const override {
				return BlockDuration();
			}
			
			void logAllProcesses() const override {}
			void logView(const dbrb::ViewData& view) const override {}
			
		private:
			dbrb::ViewData m_view;
		};

		class MockDeliverCallback {
		public:
			explicit MockDeliverCallback(std::atomic<size_t>& counter)
				: m_counter(counter)
			{}

			void operator()(const dbrb::Payload&) {
				++m_counter;
			}

		private:
			std::atomic<size_t>& m_counter;
		};

		auto CreateConfigHolder(uint32_t shardSize) {
			test::MutableBlockchainConfiguration config;
			config.Network.DbrbShardSize = shardSize;
			return std::make_shared<config::MockBlockchainConfigurationHolder>(config.ToConst());
		}

		auto CreateDbrbProcesses(
				std::vector<crypto::KeyPair>& keyPairs,
				const std::shared_ptr<thread::IoThreadPool>& pPool,
				size_t shardSize,
				size_t unreachableNodeCount,
				const dbrb::DbrbViewFetcher& dbrbViewFetcher,
				const MockDeliverCallback& deliverCallback) {
			std::map<dbrb::ProcessId, std::shared_ptr<dbrb::ShardedDbrbProcess>> processes;
			dbrb::ViewData unreachableNodes;
			for (auto i = 0u; i < unreachableNodeCount; ++i)
				unreachableNodes.emplace(keyPairs[i].publicKey());
			auto pMessageSender = std::make_shared<MockMessageSender>(pPool, processes, unreachableNodes);
			auto pConfigHolder = CreateConfigHolder(shardSize);
			dbrb::ViewData reachableNodes;
			for (auto & keyPair : keyPairs) {
				reachableNodes.emplace(keyPair.publicKey());
				auto pProcess = std::make_shared<dbrb::ShardedDbrbProcess>(keyPair, pMessageSender, pPool, nullptr, dbrbViewFetcher, shardSize);
				pProcess->updateView(pConfigHolder, Timestamp(), Height(1), false);
				pProcess->setValidationCallback([](const auto&) { return dbrb::MessageValidationResult::Message_Valid; });
				pProcess->setDeliverCallback(deliverCallback);
				pMessageSender->addProcess(pProcess);
				processes.emplace(keyPair.publicKey(), std::move(pProcess));
			}

			return processes;
		}

		auto CreatePayload() {
			auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(test::Random16());
			pPacket->Type = ionet::PacketType::Push_Precommit_Messages;
			return pPacket;
		}

		void RunTest(size_t viewSize, size_t unreachableNodeCount, size_t shardSize) {
			// Arrange:
			std::atomic<size_t> deliverCounter = 0;
			auto keyPairs = CreateKeyPairs(viewSize);
			dbrb::ViewData view;
			for (const auto& keyPair : keyPairs)
				view.emplace(keyPair.publicKey());
			MockDbrbViewFetcher dbrbViewFetcher(view);
			MockDeliverCallback deliverCallback(deliverCounter);
			auto pPool = std::shared_ptr<thread::IoThreadPool>(test::CreateStartedIoThreadPool(1));
			auto processes = CreateDbrbProcesses(keyPairs, pPool, shardSize, unreachableNodeCount, dbrbViewFetcher, deliverCallback);

			// Act:
			processes.at(keyPairs[unreachableNodeCount].publicKey())->broadcast(CreatePayload(), view);

			// Assert:
			WAIT_FOR_VALUE_SECONDS(viewSize - unreachableNodeCount, deliverCounter, 30);
		}
	}

	TEST(TEST_CLASS, PayloadDeliverySuccess1) {
		RunTest(50, 16, 4);
	}

	TEST(TEST_CLASS, PayloadDeliverySuccess2) {
		RunTest(50, 16, 5);
	}

	TEST(TEST_CLASS, PayloadDeliverySuccess3) {
		RunTest(50, 16, 6);
	}
}}
