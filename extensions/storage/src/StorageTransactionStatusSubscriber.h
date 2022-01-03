/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/subscribers/TransactionStatusSubscriber.h"
#include <memory>

namespace catapult { namespace storage { class ReplicatorService; } }

namespace catapult { namespace storage {

	/// Creates a storage transaction status subscriber around \a pReplicatorServiceWeak.
	std::unique_ptr<subscribers::TransactionStatusSubscriber> CreateStorageTransactionStatusSubscriber(
		const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak);
}}
