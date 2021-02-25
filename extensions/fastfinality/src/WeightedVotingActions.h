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
		action SelectPeers = [] {};
		action DownloadBlocks = [] {};
		action DetectStage = [] {};
		action SelectCommittee = [] {};
		action ProposeBlock = [] {};
		action WaitForProposal = [] {};
		action ValidateProposal = [] {};
		action AddPrevote = [] {};
		action WaitForPrevotes = [] {};
		action WaitForProposalPhaseEnd = [] {};
		action AddPrecommit = [] {};
		action WaitForPrecommits = [] {};
		action UpdateConfirmedBlock = [] {};
		action CommitConfirmedBlock = [] {};
		action IncrementRound = [] {};						
		action ResetRound = [] {};
		action WaitForConfirmedBlock = [] {};
	};

	action CreateDefaultCheckLocalChainAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const RemoteChainHeightsRetriever& retriever,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		const supplier<Height>& localHeightSupplier);

	action CreateDefaultResetLocalChainAction();

	action CreateDefaultSelectPeersAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const RemoteBlockHashesRetriever& retriever,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

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
		const model::BlockElementSupplier& lastBlockElementSupplier);

	action CreateDefaultWaitForProposalAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak);

	action CreateDefaultValidateProposalAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		extensions::ServiceState& state,
		const model::BlockElementSupplier& lastBlockElementSupplier,
		const std::shared_ptr<thread::IoThreadPool>& pValidatorPool);

	action CreateDefaultAddPrevoteAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak);

	action CreateDefaultWaitForProposalPhaseEndAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		const extensions::PacketPayloadSink& packetPayloadSink);

	action CreateDefaultWaitForPrevotesAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const plugins::PluginManager& pluginManager,
		const extensions::PacketPayloadSink& packetPayloadSink);

	action CreateDefaultAddPrecommitAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak);

	action CreateDefaultWaitForPrecommitsAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const plugins::PluginManager& pluginManager,
		const extensions::PacketPayloadSink& packetPayloadSink);

	action CreateDefaultUpdateConfirmedBlockAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		chain::CommitteeManager& committeeManager);

	action CreateDefaultCommitConfirmedBlockAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		consumer<model::BlockRange&&, const disruptor::ProcessingCompleteFunc&> rangeConsumer,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		const extensions::PacketPayloadSink& packetPayloadSink,
		chain::CommitteeManager& committeeManager);

	action CreateDefaultIncrementRoundAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	action CreateDefaultResetRoundAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		chain::CommitteeManager& committeeManager);

	action CreateDefaultWaitForConfirmedBlockAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const model::BlockElementSupplier& lastBlockElementSupplier);
}}