/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

namespace catapult::model {

	struct CallDigest {
		Hash256 CallId;
		bool Manual;
		bool Success;
		Hash256 ReleasedTransactionHash;
	};

	struct CallPaymentOpinion {
		std::vector<Amount> ExecutionWork;
		std::vector<Amount> DownloadWork;
	};

}