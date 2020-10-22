/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "WeightedVotingActions.h"
#include "WeightedVotingEvents.h"
#include "WeightedVotingStates.h"
#include <boost/sml.hpp>
#include <memory>

namespace sml = boost::sml;

namespace catapult { namespace fastfinality {

	struct WeightedVotingTransitionTable {
		auto operator()() const {
			auto isPhaseProposeAndIsBlockProposer = [](const auto& event) {
				return event.IsBlockProposer && (event.Phase == CommitteePhase::Propose);
			};

			auto isPhaseProposeAndIsNotBlockProposer = [](const auto& event) {
				return !event.IsBlockProposer && (event.Phase == CommitteePhase::Propose);
			};

			auto isNotPhasePropose = [](const auto& event) {
				return event.Phase != CommitteePhase::Propose;
			};

#define ACTION(NAME) [] (WeightedVotingActions& actions) { actions.NAME(); }

			return sml::make_transition_table(
				*sml::state<InitialState> + sml::event<StartLocalChainCheck> = sml::state<LocalChainCheck>,

				sml::state<LocalChainCheck> + sml::on_entry<sml::_> / ACTION(CheckLocalChain),
				sml::state<LocalChainCheck> + sml::event<NetworkHeightDetectionFailure> = sml::state<LocalChainCheck>,
				sml::state<LocalChainCheck> + sml::event<NetworkHeightLessThanLocal> = sml::state<InvalidLocalChain>,
				sml::state<LocalChainCheck> + sml::event<NetworkHeightGreaterThanLocal> = sml::state<PeersSelection>,
				sml::state<LocalChainCheck> + sml::event<NetworkHeightEqualToLocal> = sml::state<StageDetection>,

				sml::state<InvalidLocalChain> / ACTION(ResetLocalChain) = sml::X,

				sml::state<PeersSelection> + sml::on_entry<sml::_> / ACTION(SelectPeers),
				sml::state<PeersSelection> + sml::event<PeersSelectionFailed> = sml::state<PeersSelection>,
				sml::state<PeersSelection> + sml::event<PeersSelectionSucceeded> = sml::state<BlocksDownloading>,

				sml::state<BlocksDownloading> + sml::on_entry<sml::_> / ACTION(DownloadBlocks),
				sml::state<BlocksDownloading> + sml::event<DownloadBlocksFailed> = sml::state<PeersSelection>,
				sml::state<BlocksDownloading> + sml::event<DownloadBlocksSucceeded> = sml::state<LocalChainCheck>,

				sml::state<StageDetection> + sml::on_entry<sml::_> / ACTION(DetectStage),
				sml::state<StageDetection> + sml::event<StageDetectionFailed> = sml::state<StageDetection>,
				sml::state<StageDetection> + sml::event<StageDetectionSucceeded> = sml::state<CommitteeSelection>,

				sml::state<CommitteeSelection> + sml::on_entry<sml::_> / ACTION(SelectCommittee),
				sml::state<CommitteeSelection> + sml::event<CommitteeSelectionResult> [ isPhaseProposeAndIsBlockProposer ] = sml::state<BlockProposing>,
				sml::state<CommitteeSelection> + sml::event<CommitteeSelectionResult> [ isPhaseProposeAndIsNotBlockProposer ] = sml::state<ProposalWaiting>,
				sml::state<CommitteeSelection> + sml::event<CommitteeSelectionResult> [ isNotPhasePropose ] = sml::state<RoundEndWaiting>,

				sml::state<BlockProposing> + sml::on_entry<sml::_> / ACTION(ProposeBlock),
				sml::state<BlockProposing> + sml::event<BlockProposingFailed> = sml::state<RoundEndWaiting>,
				sml::state<BlockProposing> + sml::event<BlockProposingSucceeded> = sml::state<Prevote>,

				sml::state<ProposalWaiting> + sml::on_entry<sml::_> / ACTION(WaitForProposal),
				sml::state<ProposalWaiting> + sml::event<MultipleProposal> = sml::state<RoundEndWaiting>,
				sml::state<ProposalWaiting> + sml::event<ProposalNotReceived> = sml::state<RoundEndWaiting>,
				sml::state<ProposalWaiting> + sml::event<ProposalReceived> = sml::state<ProposalValidation>,

				sml::state<ProposalValidation> + sml::on_entry<sml::_> / ACTION(ValidateProposal),
				sml::state<ProposalValidation> + sml::event<ProposalInvalid> = sml::state<RoundEndWaiting>,
				sml::state<ProposalValidation> + sml::event<ProposalValid> = sml::state<Prevote>,

				sml::state<Prevote> + sml::on_entry<sml::_> / (ACTION(BroadcastPrevote), ACTION(WaitForPrevotes)),
				sml::state<Prevote> + sml::event<SumOfPrevotesInsufficient> = sml::state<RoundEndWaiting>,
				sml::state<Prevote> + sml::event<SumOfPrevotesSufficient> = sml::state<Precommit>,

				sml::state<Precommit> + sml::on_entry<sml::_> / (ACTION(BroadcastPrecommit), ACTION(WaitForPrecommits)),
				sml::state<Precommit> + sml::event<SumOfPrecommitsInsufficient> / ACTION(IncrementRound) = sml::state<CommitteeSelection>,
				sml::state<Precommit> + sml::event<SumOfPrecommitsSufficient> = sml::state<Commit>,

				sml::state<Commit> + sml::on_entry<sml::_> / ACTION(CommitConfirmedBlock),
				sml::state<Commit> + sml::event<CommitBlockFailed> / ACTION(IncrementRound) = sml::state<CommitteeSelection>,
				sml::state<Commit> + sml::event<CommitBlockSucceeded> / ACTION(ResetRound) = sml::state<CommitteeSelection>,

				sml::state<RoundEndWaiting> + sml::on_entry<sml::_> / ACTION(WaitForRoundEnd),
				sml::state<RoundEndWaiting> + sml::event<RoundEnded> = sml::state<LocalChainCheck>
			);
		}
	};
}}