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

	struct DownloadBlocksFailed{};
	struct DownloadBlocksSucceeded{};

	struct StageDetectionSucceeded{};

	struct CommitteeSelectionResult{
		bool IsBlockProposer;
		CommitteePhase Phase;
	};

	struct UnexpectedBlockHeight{};

	struct BlockProposingFailed{};
	struct BlockProposingSucceeded{};

	struct ProposalRequest{};
	struct ProposalReceived{};
	struct ProposalNotReceived{};

	struct ProposalInvalid{};
	struct ProposalValid{};

	struct ProposalPhaseEnded{};

	struct VoteRequest{};

	struct SumOfPrevotesInsufficient{};
	struct SumOfPrevotesSufficient{};

	struct SumOfPrecommitsInsufficient{};
	struct SumOfPrecommitsSufficient{};

	struct PrecommitPhaseEnded{};

	struct CommitBlockFailed{};
	struct CommitBlockSucceeded{};

	struct ConfirmedBlockRequest{};
	struct ConfirmedBlockNotReceived{};
	struct ConfirmedBlockReceived{};

	struct Stop{};
}}