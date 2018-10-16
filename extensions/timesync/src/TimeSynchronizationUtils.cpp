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

#include "TimeSynchronizationUtils.h"
#include "BalanceAwareNodeSelector.h"
#include "TimeSynchronizationConfiguration.h"
#include "TimeSynchronizationState.h"
#include "TimeSynchronizer.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/BalanceView.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/ionet/NodeContainer.h"

namespace catapult { namespace timesync {

	namespace {
		using NetworkTimeSupplier = extensions::ExtensionManager::NetworkTimeSupplier;

		BalanceAwareNodeSelector CreateBalanceAwareNodeSelector(
				const TimeSynchronizationConfiguration& timeSyncConfig,
				const config::LocalNodeConfiguration& config) {
			auto totalChainBalance = model::GetTotalBalance(config.BlockChain).unwrap();
			auto minBalance = Amount(static_cast<uint64_t>(Required_Minimum_Balance * totalChainBalance));
			return BalanceAwareNodeSelector(ionet::ServiceIdentifier(0x53594E43), timeSyncConfig.MaxNodes, minBalance);
		}

		struct SamplesResult {
		public:
			explicit SamplesResult(size_t count)
					: Samples(count)
					, NumValidSamples(0)
			{}

		public:
			std::vector<TimeSynchronizationSample> Samples;
			std::atomic<size_t> NumValidSamples;
		};

		ionet::NodeSet SelectNodes(
				const extensions::LocalNodeStateRef& localNodeState,
				const BalanceAwareNodeSelector& selector,
				const ionet::NodeContainer& nodes) {

			auto cacheView = localNodeState.Cache.createView();

			cache::BalanceView balanceView(cache::ReadOnlyAccountStateCache(cacheView.sub<cache::AccountStateCache>()));

			auto selectedNodes = selector.selectNodes(balanceView, nodes.view());
			CATAPULT_LOG(debug) << "timesync: number of selected nodes: " << selectedNodes.size();
			return selectedNodes;
		}
	}

	thread::future<TimeSynchronizationSamples> RetrieveSamples(
			const ionet::NodeSet& nodes,
			const TimeSyncResultSupplier& requestResultFutureSupplier,
			const NetworkTimeSupplier& networkTimeSupplier) {
		if (nodes.empty())
			return thread::make_ready_future<TimeSynchronizationSamples>(TimeSynchronizationSamples());

		auto pSamplesResult = std::make_shared<SamplesResult>(nodes.size());
		std::vector<thread::future<bool>> futures;
		for (const auto& node : nodes) {
			auto pLocalTimestamps = std::make_shared<CommunicationTimestamps>();
			pLocalTimestamps->SendTimestamp = networkTimeSupplier();
			auto future = requestResultFutureSupplier(node)
				.then([pSamplesResult, node, pLocalTimestamps, networkTimeSupplier](auto&& resultFuture) {
					pLocalTimestamps->ReceiveTimestamp = networkTimeSupplier();
					auto pair = resultFuture.get();
					auto& samples = pSamplesResult->Samples;
					if (net::NodeRequestResult::Success == pair.first) {
						auto index = (pSamplesResult->NumValidSamples)++;
						samples[index] = TimeSynchronizationSample(node, *pLocalTimestamps, pair.second);
						CATAPULT_LOG(info) << "'" << node << "': time offset is " << samples[index].timeOffsetToRemote();
						return true;
					}

					CATAPULT_LOG(warning)
							<< "unable to retrieve network time from node '" << node
							<< "', request result: " << pair.first;
					return false;
				});
			futures.push_back(std::move(future));
		}

		return thread::when_all(std::move(futures)).then([pSamplesResult](auto&&) {
			auto& samples = pSamplesResult->Samples;
			samples.resize(pSamplesResult->NumValidSamples);
			return TimeSynchronizationSamples(samples.cbegin(), samples.cend());
		});
	}

	thread::Task CreateTimeSyncTask(
			TimeSynchronizer& timeSynchronizer,
			const TimeSynchronizationConfiguration& timeSyncConfig,
			const TimeSyncResultSupplier& resultSupplier,
			const extensions::ServiceState& state,
			TimeSynchronizationState& timeSyncState,
			const NetworkTimeSupplier& networkTimeSupplier) {
		const extensions::LocalNodeStateRef& nodeLocalState = state.nodeLocalState();
		const auto& nodes = state.nodes();
		auto selector = CreateBalanceAwareNodeSelector(timeSyncConfig, nodeLocalState.Config);
		return thread::CreateNamedTask("time synchronization task", [&, resultSupplier, networkTimeSupplier, selector]() {
			// select nodes
			auto selectedNodes = SelectNodes(nodeLocalState, selector, nodes);

			// retrieve samples from selected nodes
			auto samplesFuture = RetrieveSamples(selectedNodes, resultSupplier, networkTimeSupplier);
			return samplesFuture.then([&timeSynchronizer, &timeSyncState, &nodeLocalState](auto&& future) {
				auto samples = future.get();
				CATAPULT_LOG(debug) << "timesync: number of retrieved samples: " << samples.size();

				// calculate new offset and update state
				auto offset = timeSynchronizer.calculateTimeOffset(nodeLocalState, std::move(samples), timeSyncState.nodeAge());
				timeSyncState.update(offset);

				return thread::TaskResult::Continue;
			});
		});
	}
}}
