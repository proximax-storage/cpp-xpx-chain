/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

namespace catapult::model {

	struct ExtendedCallDigest {
		Hash256 CallId;
		bool Manual;
		int16_t Status;
		Hash256 ReleasedTransactionHash;
	};

	struct ShortCallDigest {
		Hash256 CallId;
		bool Manual;
	};

	struct CallPaymentOpinion {
		std::vector<Amount> ExecutionWork;
		std::vector<Amount> DownloadWork;
	};

}