/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/subscribers/TransactionStatusSubscriber.h"
#include <memory>
#include "TransactionStatusHandler.h"

namespace catapult { namespace contract {

	/// Creates a storage transaction status subscriber around \a pExecutorServiceWeak.
	std::unique_ptr<subscribers::TransactionStatusSubscriber> CreateExecutorTransactionStatusSubscriber(
			std::weak_ptr<TransactionStatusHandler> pTransactionStatusHandlerWeak);
}} // namespace catapult::contract
