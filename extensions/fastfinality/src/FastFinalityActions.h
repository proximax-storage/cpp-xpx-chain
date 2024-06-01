/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "FastFinalityChainPackets.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/harvesting_core/HarvesterBlockGenerator.h"

namespace catapult {
	namespace dbrb { struct DbrbConfiguration; }
	namespace fastfinality { class FastFinalityFsm; }
}

namespace catapult { namespace fastfinality {

	struct FastFinalityActions {
		action CheckLocalChain = [] {};
		action ResetLocalChain = [] {};
		action DownloadBlocks = [] {};
		action DetectRound = [] {};
		action CheckConnections = [] {};
		action SelectBlockProducer = [] {};
		action GenerateBlock = [] {};
		action WaitForBlock = [] {};
		action CommitBlock = [] {};
		action IncrementRound = [] {};
		action ResetRound = [] {};
	};

	action CreateFastFinalityCheckLocalChainAction(
		const std::weak_ptr<FastFinalityFsm>& pFsmWeak,
		const extensions::ServiceState& state,
		const RemoteNodeStateRetriever& retriever,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		const model::BlockElementSupplier& lastBlockElementSupplier,
		const std::function<uint64_t (const Key&)>& importanceGetter,
		const dbrb::DbrbConfiguration& dbrbConfig);

	action CreateFastFinalityResetLocalChainAction();

	action CreateFastFinalityDownloadBlocksAction(
		const std::weak_ptr<FastFinalityFsm>& pFsmWeak,
		extensions::ServiceState& state,
		const consumer<model::BlockRange&&, const disruptor::ProcessingCompleteFunc&>& rangeConsumer);

	action CreateFastFinalityDetectRoundAction(
		const std::weak_ptr<FastFinalityFsm>& pFsmWeak,
		const model::BlockElementSupplier& lastBlockElementSupplier,
		extensions::ServiceState& state);

	action CreateFastFinalityCheckConnectionsAction(
		const std::weak_ptr<FastFinalityFsm>& pFsmWeak,
		extensions::ServiceState& state);

	action CreateFastFinalitySelectBlockProducerAction(
		const std::weak_ptr<FastFinalityFsm>& pFsmWeak,
		extensions::ServiceState& state);

	action CreateFastFinalityGenerateBlockAction(
		const std::weak_ptr<FastFinalityFsm>& pFsmWeak,
		const cache::CatapultCache& cache,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		const harvesting::BlockGenerator& blockGenerator,
		const model::BlockElementSupplier& lastBlockElementSupplier);

	action CreateFastFinalityWaitForBlockAction(
		const std::weak_ptr<FastFinalityFsm>& pFsmWeak,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	action CreateFastFinalityCommitBlockAction(
		const std::weak_ptr<FastFinalityFsm>& pFsmWeak,
		const consumer<model::BlockRange&&, const disruptor::ProcessingCompleteFunc&>& rangeConsumer,
		extensions::ServiceState& state);

	action CreateFastFinalityIncrementRoundAction(
		const std::weak_ptr<FastFinalityFsm>& pFsmWeak,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	action CreateFastFinalityResetRoundAction(
		const std::weak_ptr<FastFinalityFsm>& pFsmWeak,
		extensions::ServiceState& state);
}}