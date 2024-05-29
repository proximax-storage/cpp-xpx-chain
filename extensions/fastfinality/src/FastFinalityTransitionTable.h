/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "FastFinalityActions.h"
#include "FastFinalityEvents.h"
#include "FastFinalityStates.h"
#include "sml.hpp"
#include <memory>

namespace sml = boost::sml;

namespace catapult { namespace fastfinality {

	struct FastFinalityTransitionTable {
		auto operator()() const {
			auto syncWithNetwork = [](const auto& event) {
				return event.IsBroadcastStarted;
			};

			auto startNextRound = [](const auto& event) {
				return !event.IsBroadcastStarted;
			};

#define ACTION(NAME) [] (FastFinalityActions& actions) { actions.NAME(); }

			return sml::make_transition_table(
				*sml::state<InitialState> + sml::event<StartLocalChainCheck> = sml::state<LocalChainCheck>,

				*sml::state<StopWaiting> + sml::unexpected_event<sml::_> = sml::X,

				sml::state<LocalChainCheck> + sml::on_entry<sml::_> / ACTION(CheckLocalChain),
				sml::state<LocalChainCheck> + sml::event<NetworkHeightDetectionFailure> = sml::state<LocalChainCheck>,
				sml::state<LocalChainCheck> + sml::event<NetworkHeightLessThanLocal> = sml::state<InvalidLocalChain>,
				sml::state<LocalChainCheck> + sml::event<NetworkHeightGreaterThanLocal> = sml::state<BlocksDownloading>,
				sml::state<LocalChainCheck> + sml::event<NetworkHeightEqualToLocal> = sml::state<RoundDetection>,
				sml::state<LocalChainCheck> + sml::event<NotRegisteredInDbrbSystem> = sml::state<LocalChainCheck>,
				sml::state<LocalChainCheck> + sml::event<StartLocalChainCheck> = sml::state<LocalChainCheck>,

				sml::state<InvalidLocalChain> / ACTION(ResetLocalChain) = sml::X,

				sml::state<BlocksDownloading> + sml::on_entry<sml::_> / ACTION(DownloadBlocks),
				sml::state<BlocksDownloading> + sml::event<DownloadBlocksFailed> = sml::state<LocalChainCheck>,
				sml::state<BlocksDownloading> + sml::event<DownloadBlocksSucceeded> = sml::state<LocalChainCheck>,

				sml::state<RoundDetection> + sml::on_entry<sml::_> / ACTION(DetectRound),
				sml::state<RoundDetection> + sml::event<RoundDetectionSucceeded> = sml::state<ConnectionChecking>,

				sml::state<ConnectionChecking> + sml::on_entry<sml::_> / ACTION(CheckConnections),
				sml::state<ConnectionChecking> + sml::event<ConnectionNumberSufficient> = sml::state<BlockProducerSelection>,
				sml::state<ConnectionChecking> + sml::event<ConnectionNumberInsufficient> = sml::state<LocalChainCheck>,

				sml::state<BlockProducerSelection> + sml::on_entry<sml::_> / ACTION(SelectBlockProducer),
				sml::state<BlockProducerSelection> + sml::event<GenerateBlock> = sml::state<BlockGeneration>,
				sml::state<BlockProducerSelection> + sml::event<WaitForBlock> = sml::state<BlockWaiting>,
				sml::state<BlockProducerSelection> + sml::event<NotRegisteredInDbrbSystem> = sml::state<LocalChainCheck>,

				sml::state<BlockGeneration> + sml::on_entry<sml::_> / ACTION(GenerateBlock),
				sml::state<BlockGeneration> + sml::event<BlockGenerationFailed> = sml::state<BlockWaiting>,
				sml::state<BlockGeneration> + sml::event<BlockGenerationSucceeded> = sml::state<BlockWaiting>,

				sml::state<BlockWaiting> + sml::on_entry<sml::_> / ACTION(WaitForBlock),
				sml::state<BlockWaiting> + sml::event<UnexpectedBlockHeight> = sml::state<LocalChainCheck>,
				sml::state<BlockWaiting> + sml::event<BlockNotReceived> [ syncWithNetwork ] = sml::state<LocalChainCheck>,
				sml::state<BlockWaiting> + sml::event<BlockNotReceived> [ startNextRound ] / ACTION(IncrementRound) = sml::state<ConnectionChecking>,
				sml::state<BlockWaiting> + sml::event<BlockReceived> = sml::state<Commit>,

				sml::state<Commit> + sml::on_entry<sml::_> / ACTION(CommitBlock),
				sml::state<Commit> + sml::event<CommitBlockFailed> / ACTION(IncrementRound) = sml::state<ConnectionChecking>,
				sml::state<Commit> + sml::event<CommitBlockSucceeded> / ACTION(ResetRound) = sml::state<ConnectionChecking>,
				sml::state<Commit> + sml::event<Hold> = sml::state<OnHold>
			);
		}
	};
}}