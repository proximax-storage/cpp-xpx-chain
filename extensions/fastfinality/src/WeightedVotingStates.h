/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

namespace catapult { namespace fastfinality {

	struct InitialState{};

	struct StopWaiting{};

	struct LocalChainCheck{};
	struct BlocksDownloading{};

	struct InvalidLocalChain{};

	struct StageDetection{};
	struct CommitteeSelection{};
	struct BlockGeneration{};
	struct ProposalWaiting{};
	struct Prevote{};
	struct Precommit{};
	struct Commit{};
	struct ConfirmedBlockWaiting{};

	struct OnHold{};
}}