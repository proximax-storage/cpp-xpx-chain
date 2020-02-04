/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/OperationCache.h"

namespace catapult { namespace validators {

	using Notification = model::CompletedOperationNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(CompletedOperation, [](const auto& notification, const auto& context) {
		const auto& operationCache = context.Cache.template sub<cache::OperationCache>();
		if (!operationCache.contains(notification.OperationToken))
			return Failure_Operation_Token_Invalid;

		if (!operationCache.isActive(notification.OperationToken, context.Height))
			return Failure_Operation_Expired;

		auto operationCacheIter = operationCache.find(notification.OperationToken);
		const auto& operationEntry = operationCacheIter.get();

		if (operationEntry.Executors.end() == operationEntry.Executors.find(notification.Executor))
			return Failure_Operation_Invalid_Executor;

		const auto& mosaics = operationEntry.Mosaics;
		auto pMosaic = notification.MosaicsPtr;
		for (auto i = 0u; i < notification.MosaicCount; ++i, ++pMosaic) {
			auto mosaicId = context.Resolvers.resolve(pMosaic->MosaicId);
			auto mosaicIter = mosaics.find(mosaicId);
			if (mosaics.end() == mosaicIter)
				return Failure_Operation_Mosaic_Invalid;
			if (pMosaic->Amount > mosaicIter->second)
				return Failure_Operation_Invalid_Mosaic_Amount;
		}

		return ValidationResult::Success;
	})
}}
