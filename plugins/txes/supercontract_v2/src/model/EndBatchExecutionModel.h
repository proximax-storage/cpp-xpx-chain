/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

namespace catapult::model {

struct CallPayment {
	Amount ExecutionPayment;
	Amount DownloadPayment;
};

struct RawProofOfExecution {
	uint64_t StartBatchId;
	std::array<uint8_t, 32> T;
	std::array<uint8_t, 32> R;
	std::array<uint8_t, 32> F;
	std::array<uint8_t, 32> K;
};

}