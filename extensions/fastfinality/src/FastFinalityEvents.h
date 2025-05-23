/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

namespace catapult { namespace fastfinality {

	struct StartLocalChainCheck{};

	struct NetworkHeightDetectionFailure{};

	struct NetworkHeightLessThanLocal{};
	struct NetworkHeightGreaterThanLocal{};
	struct NetworkHeightEqualToLocal{};

	struct DownloadBlocksFailed{};
	struct DownloadBlocksSucceeded{};

	struct RoundDetectionCompleted{};

	struct ConnectionNumberSufficient{};
	struct ConnectionNumberInsufficient{};

	struct RoundStarted{};
	struct GenerateBlock{};
	struct WaitForBlock{};
	struct NotRegisteredInDbrbSystem{};
	struct DbrbProcessBanned{};

	struct UnexpectedBlockHeight{};

	struct BlockGenerationFailed{};
	struct BlockGenerationSucceeded{};

	struct SelectNextBlockProducer{};
	struct BlockNotReceived{
		bool SyncWithNetwork = false;
	};
	struct BlockReceived{};

	struct UnexpectedBlock{};
	struct CommitBlockFailed{};
	struct CommitBlockSucceeded{};

	struct Stop{};

	struct Hold{};
}}