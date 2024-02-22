/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DbrbViewCache.h"
#include "catapult/crypto/Signer.h"
#include "catapult/dbrb/DbrbUtils.h"
#include "catapult/dbrb/View.h"

namespace catapult { namespace validators {

	using Notification = model::RemoveDbrbProcessByNetworkNotification<1>;

	namespace {
		bool IsDbrbProcessExpiredOrNotExist(const dbrb::ProcessId processId, const Timestamp& timestamp, const cache::DbrbViewCache::CacheReadOnlyType& cache, const dbrb::ViewData& bootstrapProcesses) {
			if (bootstrapProcesses.find(processId) != bootstrapProcesses.cend())
				return false;

			auto iter = cache.find(processId);
			auto pEntry = iter.tryGet();

			return (!pEntry || pEntry->expirationTime() < timestamp);
		}
	}

	DECLARE_STATEFUL_VALIDATOR(RemoveDbrbProcessByNetwork, Notification)(const dbrb::DbrbViewFetcher& dbrbViewFetcher) {
		return MAKE_STATEFUL_VALIDATOR(RemoveDbrbProcessByNetwork, ([&dbrbViewFetcher](const Notification& notification, const ValidatorContext& context) {
			if (notification.Timestamp > context.BlockTime + context.Config.Network.MaxBlockFutureTime)
					return Failure_Dbrb_Node_Removal_Too_Far_In_Future;

			if (notification.Timestamp < utils::SubtractNonNegative(context.BlockTime, context.Config.Network.MaxTransactionLifetime))
					return Failure_Dbrb_Node_Removal_Too_Far_In_Past;

			const auto& cache = context.Cache.sub<cache::DbrbViewCache>();
			if (IsDbrbProcessExpiredOrNotExist(notification.ProcessId, notification.Timestamp, cache, context.Config.Network.DbrbBootstrapProcesses))
				return Failure_Dbrb_Node_Removal_Subject_Is_Not_In_Dbrb_System;

			auto view = dbrb::View{ dbrbViewFetcher.getView(notification.Timestamp) };
			auto bootstrapView = dbrb::View{ context.Config.Network.DbrbBootstrapProcesses };
			view.merge(bootstrapView);
			view.Data.erase(notification.ProcessId);
			// Add transaction signer's vote.
			if (notification.VoteCount + 1 < view.quorumSize())
				return Failure_Dbrb_Node_Removal_Not_Enough_Votes;

			auto hash = dbrb::CalculateHash({ { reinterpret_cast<const uint8_t*>(&notification.Timestamp), sizeof(Timestamp) }, { notification.ProcessId.data(), Key_Size } });
			auto pVote = notification.VotesPtr;
			for (auto i = 0u; i < notification.VoteCount; ++i, ++pVote) {
				if (IsDbrbProcessExpiredOrNotExist(pVote->Signer, notification.Timestamp, cache, context.Config.Network.DbrbBootstrapProcesses))
					return Failure_Dbrb_Node_Removal_Voter_Is_Not_In_Dbrb_System;

				if (!crypto::Verify(pVote->Signer, hash, pVote->Signature))
					return Failure_Dbrb_Node_Removal_Invalid_Signature;
			}

			return ValidationResult::Success;
		}));
	}
}}
