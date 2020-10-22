/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteeStage.h"
#include "WeightedVotingHandlers.h"
#include "WeightedVotingChainPackets.h"
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
		action BroadcastPrevote = [] {};
		action WaitForPrevotes = [] {};
		action BroadcastPrecommit = [] {};
		action WaitForPrecommits = [] {};
		action CommitConfirmedBlock = [] {};
		action IncrementRound = [] {};						
		action ResetRound = [] {};
		action WaitForRoundEnd = [] {};
	};

	action CreateDefaultCheckLocalChainAction(
		const std::shared_ptr<WeightedVotingFsm>& pFsm,
		const extensions::RemoteChainHeightsRetriever& retriever,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		const supplier<Height>& localHeightSupplier);

	action CreateDefaultResetLocalChainAction();

	action CreateDefaultSelectPeersAction(
		const std::shared_ptr<WeightedVotingFsm>& pFsm,
		const RemoteBlockHashesIoRetriever& retriever,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	action CreateDefaultDownloadBlocksAction(
		const std::shared_ptr<WeightedVotingFsm>& pFsm,
		extensions::ServiceState& state);

	action CreateDefaultDetectStageAction(
		const std::shared_ptr<WeightedVotingFsm>& pFsm,
		const RemoteCommitteeStagesRetriever& retriever,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		const chain::TimeSupplier& timeSupplier,
		const model::BlockElementSupplier& lastBlockElementSupplier);

	action CreateDefaultSelectCommitteeAction(
		const std::shared_ptr<WeightedVotingFsm>& pFsm,
		chain::CommitteeManager& committeeManager,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	action CreateDefaultProposeBlockAction(
		const std::shared_ptr<WeightedVotingFsm>& pFsm,
		const cache::CatapultCache& cache,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		const harvesting::BlockGenerator& blockGenerator,
		const model::BlockElementSupplier& lastBlockElementSupplier,
		const extensions::PacketPayloadSink& packetPayloadSink);

	action CreateDefaultWaitForProposalAction(
		const std::shared_ptr<WeightedVotingFsm>& pFsm,
		const RemoteProposedBlockRetriever& retriever,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	action CreateDefaultValidateProposalAction(
		const std::shared_ptr<WeightedVotingFsm>& pFsm,
		extensions::ServiceState& state,
		const model::BlockElementSupplier& lastBlockElementSupplier,
		const std::shared_ptr<thread::IoThreadPool>& pValidatorPool);

	action CreateDefaultBroadcastPrevoteAction(
		const std::shared_ptr<WeightedVotingFsm>& pFsm,
		const extensions::PacketPayloadSink& packetPayloadSink);

	action CreateDefaultWaitForPrevotesAction(
		const std::shared_ptr<WeightedVotingFsm>& pFsm,
		const plugins::PluginManager& pluginManager);

	action CreateDefaultBroadcastPrecommitAction(
		const std::shared_ptr<WeightedVotingFsm>& pFsm,
		const extensions::PacketPayloadSink& packetPayloadSink);

	action CreateDefaultWaitForPrecommitsAction(
		const std::shared_ptr<WeightedVotingFsm>& pFsm,
		const plugins::PluginManager& pluginManager);

	action CreateDefaultCommitConfirmedBlockAction(
		const std::shared_ptr<WeightedVotingFsm>& pFsm,
		consumer<model::BlockRange&&, const disruptor::ProcessingCompleteFunc&> rangeConsumer);

	action CreateDefaultIncrementRoundAction(
		const std::shared_ptr<WeightedVotingFsm>& pFsm,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	action CreateDefaultResetRoundAction(
		const std::shared_ptr<WeightedVotingFsm>& pFsm,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	action CreateDefaultWaitForRoundEndAction(
		const std::shared_ptr<WeightedVotingFsm>& pFsm);
}}