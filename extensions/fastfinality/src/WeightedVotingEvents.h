/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteePhase.h"
#include "catapult/types.h"

namespace catapult { namespace fastfinality {

	struct StartLocalChainCheck{};

	struct NetworkHeightDetectionFailure{};

	struct NetworkHeightLessThanLocal{};
	struct NetworkHeightGreaterThanLocal{};
	struct NetworkHeightEqualToLocal{};

	struct PeersSelectionFailed{};
	struct PeersSelectionSucceeded{};

	struct DownloadBlocksFailed{};
	struct DownloadBlocksSucceeded{};

	struct StageDetectionFailed{};
	struct StageDetectionSucceeded{};

	struct CommitteeSelectionResult{
		bool IsBlockProposer;
		CommitteePhase Phase;
	};

	struct BlockProposingFailed{};
	struct BlockProposingSucceeded{};

	struct MultipleProposal{};
	struct ProposalNotReceived{};
	struct ProposalReceived{};

	struct ProposalInvalid{};
	struct ProposalValid{};

	struct SumOfPrevotesInsufficient{};
	struct SumOfPrevotesSufficient{};

	struct SumOfPrecommitsInsufficient{};
	struct SumOfPrecommitsSufficient{};

	struct CommitBlockFailed{};
	struct CommitBlockSucceeded{};

	struct RoundEnded{};
}}