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
#include "ImportanceAwareNodeSelector.h"
#include "TimeSynchronizationConfiguration.h"
#include "TimeSynchronizationState.h"
#include "TimeSynchronizer.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/ImportanceView.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/ionet/NodeContainer.h"

namespace catapult { namespace timesync {

	namespace {
		using NetworkTimeSupplier = extensions::ExtensionManager::NetworkTimeSupplier;

		ImportanceAwareNodeSelector CreateImportanceAwareNodeSelector(
				const TimeSynchronizationConfiguration& timeSyncConfig,
				const config::BlockchainConfiguration& config) {
			auto totalChainImportance = config.Network.TotalChainImportance.unwrap();
			auto minImportance = Importance(static_cast<uint64_t>(Required_Minimum_Importance * totalChainImportance));
			return ImportanceAwareNodeSelector(ionet::ServiceIdentifier(0x53594E43), timeSyncConfig.MaxNodes, minImportance);
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
				const cache::AccountStateCache& cache,
				const ImportanceAwareNodeSelector& selector,
				const ionet::NodeContainer& nodes,
				Height height) {
			auto view = cache.createView(height);
			cache::ImportanceView importanceView(view->asReadOnly());
			auto selectedNodes = selector.selectNodes(importanceView, nodes.view(), height);
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
			TimeSynchronizationState& timeSyncState) {
		return thread::CreateNamedTask("time synchronization task", [&, timeSyncConfig, resultSupplier]() {;
			auto height = state.cache().height();
			auto selector = CreateImportanceAwareNodeSelector(timeSyncConfig, state.config(height));

			// select nodes
			const auto& cache = state.cache().sub<cache::AccountStateCache>();
			auto selectedNodes = SelectNodes(cache, selector, state.nodes(), height);

			// retrieve samples from selected nodes
			auto samplesFuture = RetrieveSamples(selectedNodes, resultSupplier, state.timeSupplier());
			return samplesFuture.then([&timeSynchronizer, &timeSyncState, &cache, height](auto&& future) {
				auto samples = future.get();
				CATAPULT_LOG(debug) << "timesync: number of retrieved samples: " << samples.size();

				// calculate new offset and update state
				auto view = cache.createView(height);
				auto offset = timeSynchronizer.calculateTimeOffset(*view, height, std::move(samples), timeSyncState.nodeAge());
				timeSyncState.update(offset);

				return thread::TaskResult::Continue;
			});
		});
	}
}}
