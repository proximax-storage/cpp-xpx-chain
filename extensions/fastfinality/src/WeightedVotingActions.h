/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteeStage.h"
#include "WeightedVotingHandlers.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/functions.h"
#include "catapult/harvesting_core/HarvesterBlockGenerator.h"

namespace catapult { namespace fastfinality { class WeightedVotingFsm; } }

namespace catapult { namespace fastfinality {

	struct WeightedVotingActions {
		action CheckLocalChain = [] {};
		action ResetLocalChain = [] {};
		action DownloadBlocks = [] {};
		action DetectStage = [] {};
		action SelectCommittee = [] {};
		action ProposeBlock = [] {};
		action RequestProposal = [] {};
		action ValidateProposal = [] {};
		action AddPrevote = [] {};
		action RequestPrevotes = [] {};
		action WaitForProposalPhaseEnd = [] {};
		action AddPrecommit = [] {};
		action RequestPrecommits = [] {};
		action WaitForPrecommitPhaseEnd = [] {};
		action UpdateConfirmedBlock = [] {};
		action CommitConfirmedBlock = [] {};
		action IncrementRound = [] {};						
		action ResetRound = [] {};
		action RequestConfirmedBlock = [] {};
	};

	action CreateDefaultCheckLocalChainAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const RemoteNodeStateRetriever& retriever,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		const model::BlockElementSupplier& lastBlockElementSupplier,
		const std::function<uint64_t (const Key&)>& importanceGetter,
		const chain::CommitteeManager& committeeManager);

	action CreateDefaultResetLocalChainAction();

	action CreateDefaultDownloadBlocksAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		extensions::ServiceState& state,
		consumer<model::BlockRange&&, const disruptor::ProcessingCompleteFunc&> rangeConsumer,
		chain::CommitteeManager& committeeManager);

	action CreateDefaultDetectStageAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		const chain::TimeSupplier& timeSupplier,
		const model::BlockElementSupplier& lastBlockElementSupplier,
		chain::CommitteeManager& committeeManager);

	action CreateDefaultSelectCommitteeAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		chain::CommitteeManager& committeeManager,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		const chain::TimeSupplier& timeSupplier);

	action CreateDefaultProposeBlockAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const cache::CatapultCache& cache,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		const harvesting::BlockGenerator& blockGenerator,
		const model::BlockElementSupplier& lastBlockElementSupplier,
		const extensions::PacketPayloadSink& packetPayloadSink);

	action CreateDefaultRequestProposalAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		extensions::ServiceState& state);

	action CreateDefaultValidateProposalAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		extensions::ServiceState& state,
		const model::BlockElementSupplier& lastBlockElementSupplier,
		const std::shared_ptr<thread::IoThreadPool>& pValidatorPool);

	action CreateDefaultAddPrevoteAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const extensions::PacketPayloadSink& packetPayloadSink);

	action CreateDefaultWaitForProposalPhaseEndAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	action CreateDefaultRequestPrevotesAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const plugins::PluginManager& pluginManager);

	action CreateDefaultAddPrecommitAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const extensions::PacketPayloadSink& packetPayloadSink);

	action CreateDefaultRequestPrecommitsAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const plugins::PluginManager& pluginManager);

	action CreateDefaultWaitForPrecommitPhaseEndAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	action CreateDefaultUpdateConfirmedBlockAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		chain::CommitteeManager& committeeManager);

	action CreateDefaultCommitConfirmedBlockAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		consumer<model::BlockRange&&, const disruptor::ProcessingCompleteFunc&> rangeConsumer,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		chain::CommitteeManager& committeeManager);

	action CreateDefaultIncrementRoundAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	action CreateDefaultResetRoundAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		chain::CommitteeManager& committeeManager);

	action CreateDefaultRequestConfirmedBlockAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		extensions::ServiceState& state,
		const model::BlockElementSupplier& lastBlockElementSupplier);
}}