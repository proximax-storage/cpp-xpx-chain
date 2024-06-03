/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteeRound.h"
#include "WeightedVotingHandlers.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/functions.h"
#include "catapult/harvesting_core/HarvesterBlockGenerator.h"

namespace catapult {
	namespace dbrb { struct DbrbConfiguration; }
	namespace fastfinality { class WeightedVotingFsm; }
}

namespace catapult { namespace fastfinality {

	struct WeightedVotingActions {
		action CheckLocalChain = [] {};
		action ResetLocalChain = [] {};
		action DownloadBlocks = [] {};
		action DetectStage = [] {};
		action SelectCommittee = [] {};
		action ProposeBlock = [] {};
		action AddPrevote = [] {};
		action WaitForProposal = [] {};
		action WaitForPrevotes = [] {};
		action AddPrecommit = [] {};
		action WaitForPrecommits = [] {};
		action UpdateConfirmedBlock = [] {};
		action WaitForConfirmedBlock = [] {};
		action CommitConfirmedBlock = [] {};
		action IncrementRound = [] {};						
		action ResetRound = [] {};
	};

	action CreateWeightedVotingCheckLocalChainAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		const RemoteNodeStateRetriever& retriever,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		const model::BlockElementSupplier& lastBlockElementSupplier,
		const std::function<uint64_t (const Key&)>& importanceGetter,
		const dbrb::DbrbConfiguration& dbrbConfig);

	action CreateWeightedVotingResetLocalChainAction();

	action CreateWeightedVotingDownloadBlocksAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		extensions::ServiceState& state,
		const consumer<model::BlockRange&&, const disruptor::ProcessingCompleteFunc&>& rangeConsumer);

	action CreateWeightedVotingDetectStageAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		const chain::TimeSupplier& timeSupplier,
		const model::BlockElementSupplier& lastBlockElementSupplier,
		extensions::ServiceState& state);

	action CreateWeightedVotingSelectCommitteeAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		extensions::ServiceState& state);

	action CreateWeightedVotingProposeBlockAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		const cache::CatapultCache& cache,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		const harvesting::BlockGenerator& blockGenerator,
		const model::BlockElementSupplier& lastBlockElementSupplier);

	action CreateWeightedVotingAddPrevoteAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak);

	action CreateWeightedVotingWaitForProposalAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak);

	action CreateWeightedVotingWaitForPrevotesAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak);

	action CreateWeightedVotingAddPrecommitAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak);

	action CreateWeightedVotingWaitForPrecommitsAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak);

	action CreateWeightedVotingUpdateConfirmedBlockAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		extensions::ServiceState& state);

	action CreateWeightedVotingWaitForConfirmedBlockAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		extensions::ServiceState& state);

	action CreateWeightedVotingCommitConfirmedBlockAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		const consumer<model::BlockRange&&, const disruptor::ProcessingCompleteFunc&>& rangeConsumer,
		extensions::ServiceState& state);

	action CreateWeightedVotingIncrementRoundAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	action CreateWeightedVotingResetRoundAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		extensions::ServiceState& state);
}}