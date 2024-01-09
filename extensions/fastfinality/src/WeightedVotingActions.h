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
		action ValidateProposal = [] {};
		action AddPrevote = [] {};
		action WaitForProposal = [] {};
		action WaitForPrevotePhaseEnd = [] {};
		action AddPrecommit = [] {};
		action WaitForPrecommitPhaseEnd = [] {};
		action UpdateConfirmedBlock = [] {};
		action RequestConfirmedBlock = [] {};
		action CommitConfirmedBlock = [] {};
		action IncrementRound = [] {};						
		action ResetRound = [] {};
	};

	action CreateDefaultCheckLocalChainAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		const RemoteNodeStateRetriever& retriever,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		const model::BlockElementSupplier& lastBlockElementSupplier,
		const std::function<uint64_t (const Key&)>& importanceGetter);

	action CreateDefaultResetLocalChainAction();

	action CreateDefaultDownloadBlocksAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		extensions::ServiceState& state,
		const consumer<model::BlockRange&&, const disruptor::ProcessingCompleteFunc&>& rangeConsumer);

	action CreateDefaultDetectStageAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		const chain::TimeSupplier& timeSupplier,
		const model::BlockElementSupplier& lastBlockElementSupplier,
		extensions::ServiceState& state);

	action CreateDefaultSelectCommitteeAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		extensions::ServiceState& state);

	action CreateDefaultProposeBlockAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		const cache::CatapultCache& cache,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		const harvesting::BlockGenerator& blockGenerator,
		const model::BlockElementSupplier& lastBlockElementSupplier);

	action CreateDefaultValidateProposalAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		extensions::ServiceState& state,
		const model::BlockElementSupplier& lastBlockElementSupplier,
		const std::shared_ptr<thread::IoThreadPool>& pValidatorPool);

	action CreateDefaultAddPrevoteAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak);

	action CreateDefaultWaitForProposalAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak);

	action CreateDefaultWaitForPrevotePhaseEndAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		extensions::ServiceState& state);

	action CreateDefaultAddPrecommitAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak);

	action CreateDefaultWaitForPrecommitPhaseEndAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		extensions::ServiceState& state);

	action CreateDefaultUpdateConfirmedBlockAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		extensions::ServiceState& state);

	action CreateDefaultRequestConfirmedBlockAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		extensions::ServiceState& state,
		const model::BlockElementSupplier& lastBlockElementSupplier);

	action CreateDefaultCommitConfirmedBlockAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		const consumer<model::BlockRange&&, const disruptor::ProcessingCompleteFunc&>& rangeConsumer,
		extensions::ServiceState& state);

	action CreateDefaultIncrementRoundAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	action CreateDefaultResetRoundAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		extensions::ServiceState& state);
}}