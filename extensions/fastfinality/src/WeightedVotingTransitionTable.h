/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "WeightedVotingActions.h"
#include "WeightedVotingEvents.h"
#include "WeightedVotingStates.h"
#include "sml.hpp"
#include <memory>

namespace sml = boost::sml;

namespace catapult { namespace fastfinality {

	struct WeightedVotingTransitionTable {
		auto operator()() const {
			auto isPhaseProposeAndIsBlockProposer = [](const auto& event) {
				return event.IsBlockProposer && (event.Phase == CommitteePhase::Propose);
			};

			auto isPhaseProposeAndIsCosigner = [](const auto& event) {
				return event.IsCosigner && (event.Phase == CommitteePhase::Propose);
			};

			auto isNotPhaseProposeOrNotInCommittee = [](const auto& event) {
				return (event.Phase != CommitteePhase::Propose) || !(event.IsBlockProposer || event.IsCosigner);
			};

#define ACTION(NAME) [] (WeightedVotingActions& actions) { actions.NAME(); }

			return sml::make_transition_table(
				*sml::state<InitialState> + sml::event<StartLocalChainCheck> = sml::state<LocalChainCheck>,

				*sml::state<StopWaiting> + sml::unexpected_event<sml::_> = sml::X,

				sml::state<LocalChainCheck> + sml::on_entry<sml::_> / ACTION(CheckLocalChain),
				sml::state<LocalChainCheck> + sml::event<NetworkHeightDetectionFailure> = sml::state<LocalChainCheck>,
				sml::state<LocalChainCheck> + sml::event<NetworkHeightLessThanLocal> = sml::state<InvalidLocalChain>,
				sml::state<LocalChainCheck> + sml::event<NetworkHeightGreaterThanLocal> = sml::state<BlocksDownloading>,
				sml::state<LocalChainCheck> + sml::event<NetworkHeightEqualToLocal> = sml::state<StageDetection>,
				sml::state<LocalChainCheck> + sml::event<NotRegisteredInDbrbSystem> = sml::state<LocalChainCheck>,

				sml::state<InvalidLocalChain> / ACTION(ResetLocalChain) = sml::X,

				sml::state<BlocksDownloading> + sml::on_entry<sml::_> / ACTION(DownloadBlocks),
				sml::state<BlocksDownloading> + sml::event<DownloadBlocksFailed> = sml::state<LocalChainCheck>,
				sml::state<BlocksDownloading> + sml::event<DownloadBlocksSucceeded> = sml::state<LocalChainCheck>,

				sml::state<StageDetection> + sml::on_entry<sml::_> / ACTION(DetectStage),
				sml::state<StageDetection> + sml::event<StageDetectionSucceeded> = sml::state<CommitteeSelection>,

				sml::state<CommitteeSelection> + sml::on_entry<sml::_> / ACTION(SelectCommittee),
				sml::state<CommitteeSelection> + sml::event<CommitteeSelectionResult> [ isPhaseProposeAndIsBlockProposer ] = sml::state<BlockGeneration>,
				sml::state<CommitteeSelection> + sml::event<CommitteeSelectionResult> [ isPhaseProposeAndIsCosigner ] = sml::state<ProposalWaiting>,
				sml::state<CommitteeSelection> + sml::event<CommitteeSelectionResult> [ isNotPhaseProposeOrNotInCommittee ] = sml::state<ConfirmedBlockWaiting>,
				sml::state<CommitteeSelection> + sml::event<NotEnoughBootKeys> = sml::state<ConfirmedBlockWaiting>,
				sml::state<CommitteeSelection> + sml::event<NotRegisteredInDbrbSystem> = sml::state<LocalChainCheck>,

				sml::state<BlockGeneration> + sml::on_entry<sml::_> / ACTION(ProposeBlock),
				sml::state<BlockGeneration> + sml::event<BlockGenerationFailed> = sml::state<ConfirmedBlockWaiting>,
				sml::state<BlockGeneration> + sml::event<BlockGenerationSucceeded> = sml::state<ProposalWaiting>,

				sml::state<ProposalWaiting> + sml::on_entry<sml::_> / ACTION(WaitForProposal),
				sml::state<ProposalWaiting> + sml::event<UnexpectedBlockHeight> = sml::state<LocalChainCheck>,
				sml::state<ProposalWaiting> + sml::event<ProposalNotReceived> = sml::state<ConfirmedBlockWaiting>,
				sml::state<ProposalWaiting> + sml::event<ProposalReceived> / ACTION(AddPrevote) = sml::state<Prevote>,

				sml::state<Prevote> + sml::on_entry<sml::_> / ACTION(WaitForPrevotes),
				sml::state<Prevote> + sml::event<SumOfPrevotesInsufficient> = sml::state<ConfirmedBlockWaiting>,
				sml::state<Prevote> + sml::event<SumOfPrevotesSufficient> / ACTION(AddPrecommit) = sml::state<Precommit>,

				sml::state<Precommit> + sml::on_entry<sml::_> / ACTION(WaitForPrecommits),
				sml::state<Precommit> + sml::event<SumOfPrecommitsInsufficient> = sml::state<ConfirmedBlockWaiting>,
				sml::state<Precommit> + sml::event<SumOfPrecommitsSufficient> / ACTION(UpdateConfirmedBlock) = sml::state<ConfirmedBlockWaiting>,

				sml::state<ConfirmedBlockWaiting> + sml::on_entry<sml::_> / ACTION(WaitForConfirmedBlock),
				sml::state<ConfirmedBlockWaiting> + sml::event<UnexpectedBlockHeight> = sml::state<LocalChainCheck>,
				sml::state<ConfirmedBlockWaiting> + sml::event<ConfirmedBlockNotReceived> / ACTION(IncrementRound) = sml::state<CommitteeSelection>,
				sml::state<ConfirmedBlockWaiting> + sml::event<ConfirmedBlockReceived> = sml::state<Commit>,

				sml::state<Commit> + sml::on_entry<sml::_> / ACTION(CommitConfirmedBlock),
				sml::state<Commit> + sml::event<CommitBlockFailed> / ACTION(IncrementRound) = sml::state<CommitteeSelection>,
				sml::state<Commit> + sml::event<CommitBlockSucceeded> / ACTION(ResetRound) = sml::state<CommitteeSelection>,
				sml::state<Commit> + sml::event<Hold> = sml::state<OnHold>
			);
		}
	};
}}