/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/catapult/model/Address.h"
#include "Validators.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/model/MosaicLevy.h"
#include "src/utils/MosaicLevyCalculator.h"
#include "src/utils/MosaicLevyUtils.h"

namespace catapult {
	namespace validators {

		using Notification = model::BalanceTransferNotification<1>;

		namespace {
			bool FindAccountBalance(const cache::ReadOnlyAccountStateCache& cache, const Key& publicKey, MosaicId mosaicId, Amount& amount) {
				auto accountStateKeyIter = cache.find(publicKey);
				if (accountStateKeyIter.tryGet()) {
					amount = accountStateKeyIter.get().Balances.get(mosaicId);
					return true;
				}

				// if state could not be accessed by public key, try searching by address
				auto accountStateAddressIter = cache.find(model::PublicKeyToAddress(publicKey, cache.networkIdentifier()));
				if (accountStateAddressIter.tryGet()) {
					amount = accountStateAddressIter.get().Balances.get(mosaicId);
					return true;
				}

				return false;
			}
		}

		ValidationResult LevyTransferValidatorDetail(const Notification &notification, const ValidatorContext &context) {
			/// 1. check if levy setup exist
			auto mosaicId = context.Resolvers.resolve(notification.MosaicId);
			auto pLevy = GetLevy(mosaicId, context);
			if (!pLevy)
				return ValidationResult::Success;

			/// 2. check if levy mosaic Id is valid
			if (!utils::IsMosaicIdValid(pLevy->MosaicId, context))
				return Failure_Mosaic_Levy_Not_Found_Or_Expired;

			/// 3. compute for levy fee
			utils::MosaicLevyCalculatorFactory factory;
			Amount requiredAmount = factory.getCalculator(pLevy->Type)(notification.Amount, pLevy->Fee);

			/// 4. check transferred mosaic
			/// if transferred mosaicId and levy mosaicId are equal
			/// then the account should have the sum of levy plus transferred amount
			if (notification.MosaicId == UnresolvedMosaicId(pLevy->MosaicId.unwrap()))
				requiredAmount = requiredAmount + notification.Amount;

			/// 5. check if user has enough balance to pay for free
			Amount amount;
			const auto &accountCache = context.Cache.sub<cache::AccountStateCache>();
			if (!FindAccountBalance(accountCache, notification.Sender, pLevy->MosaicId, amount)
				|| amount < requiredAmount)
				return Failure_Mosaic_Insufficient_Levy_Balance;

			return ValidationResult::Success;
		}

		DECLARE_STATEFUL_VALIDATOR(LevyTransfer, Notification)() {
			return MAKE_STATEFUL_VALIDATOR(LevyTransfer, [](const auto &notification, const auto &context) {
				return LevyTransferValidatorDetail(notification, context);
			});
		}
	}
}
