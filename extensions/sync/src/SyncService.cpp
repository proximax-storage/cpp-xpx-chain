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

#include "SyncService.h"
#include "NetworkPacketWritersService.h"
#include "catapult/api/LocalChainApi.h"
#include "catapult/api/RemoteChainApi.h"
#include "catapult/api/RemoteTransactionApi.h"
#include "catapult/chain/UtSynchronizer.h"
#include "catapult/extensions/LocalNodeChainScore.h"
#include "catapult/extensions/PeersConnectionTasks.h"
#include "catapult/extensions/SynchronizerTaskCallbacks.h"

namespace catapult { namespace sync {

	namespace {
		constexpr auto Sync_Source = disruptor::InputSource::Remote_Pull;
		constexpr auto Service_Id = ionet::ServiceIdentifier(0x53594E43);

		thread::Task CreateConnectPeersTask(extensions::ServiceState& state, net::PacketWriters& packetWriters) {
			auto settings = extensions::SelectorSettings(
					state,
					Service_Id);
			auto task = extensions::CreateConnectPeersTask(settings, packetWriters);
			task.Name += " for service Sync";
			return task;
		}

		chain::ChainSynchronizerConfiguration CreateChainSynchronizerConfiguration(const config::BlockchainConfiguration& config) {
			chain::ChainSynchronizerConfiguration chainSynchronizerConfig;
			chainSynchronizerConfig.MaxBlocksPerSyncAttempt = config.Node.MaxBlocksPerSyncAttempt;
			chainSynchronizerConfig.MaxChainBytesPerSyncAttempt = config.Node.MaxChainBytesPerSyncAttempt.bytes32();
			return chainSynchronizerConfig;
		}

		thread::Task CreateSynchronizerTask(extensions::ServiceState& state, std::weak_ptr<net::PacketWriters> pPacketWriters) {
			const auto& config = state.config();
			auto chainSynchronizer = chain::CreateChainSynchronizer(
					api::CreateLocalChainApi(state.storage(), [&score = state.score()]() {
						return score.get();
					}),
					CreateChainSynchronizerConfiguration(config),
					state,
					state.hooks().completionAwareBlockRangeConsumerFactory()(Sync_Source));

			thread::Task task;
			task.Name = "synchronizer task";
			task.Callback = CreateSynchronizerTaskCallback(
					std::move(chainSynchronizer),
					api::CreateRemoteChainApi,
					pPacketWriters,
					state,
					task.Name);
			return task;
		}

		thread::Task CreatePullUtTask(const extensions::ServiceState& state, std::weak_ptr<net::PacketWriters> pPacketWriters) {
			auto utSynchronizer = chain::CreateUtSynchronizer(
					[pConfigHolder = state.pluginManager().configHolder()]() {
						return config::GetMinFeeMultiplier(pConfigHolder->Config());
					},
					[&cache = state.utCache()]() { return cache.view().shortHashes(); },
					state.hooks().transactionRangeConsumerFactory()(Sync_Source));

			thread::Task task;
			task.Name = "pull unconfirmed transactions task";
			task.Callback = CreateChainSyncAwareSynchronizerTaskCallback(
					std::move(utSynchronizer),
					api::CreateRemoteTransactionApi,
					pPacketWriters,
					state,
					task.Name);
			return task;
		}

		class SyncServiceRegistrar : public extensions::ServiceRegistrar {
		public:
			extensions::ServiceRegistrarInfo info() const override {
				return { "Sync", extensions::ServiceRegistrarPhase::Post_Range_Consumers };
			}

			void registerServiceCounters(extensions::ServiceLocator&) override {
				// no additional counters
			}

			void registerServices(extensions::ServiceLocator& locator, extensions::ServiceState& state) override {
				auto pPacketWriters = GetPacketWriters(locator);

				// add tasks
				state.tasks().push_back(CreateConnectPeersTask(state, *pPacketWriters));
				state.tasks().push_back(CreateSynchronizerTask(state, pPacketWriters));
				state.tasks().push_back(CreatePullUtTask(state, pPacketWriters));
			}
		};
	}

	DECLARE_SERVICE_REGISTRAR(Sync)() {
		return std::make_unique<SyncServiceRegistrar>();
	}
}}
