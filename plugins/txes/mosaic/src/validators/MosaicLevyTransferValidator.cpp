#include <src/catapult/model/Address.h>
#include "Validators.h"
#include "ActiveMosaicView.h"
#include "src/cache/MosaicCache.h"
#include "catapult/cache/ReadOnlyCatapultCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/model/MosaicLevy.h"
#include "src/utils/MosaicLevyCalculator.h"

namespace catapult { namespace validators {

	using Notification = model::BalanceTransferNotification<1>;

	namespace {
		bool IsBalanceEnough(const cache::ReadOnlyAccountStateCache& cache, const Key& publicKey,
				MosaicId mosaicId, const Amount& amount) {

			auto accountStateKeyIter = cache.find(publicKey);
			if (accountStateKeyIter.tryGet()) {
				auto balance = accountStateKeyIter.get().Balances.get(mosaicId);
				return balance >= amount;
			}

			// if state could not be accessed by public key, try searching by address
			auto accountStateAddressIter = cache.find(model::PublicKeyToAddress(publicKey, cache.networkIdentifier()));
			if (accountStateAddressIter.tryGet()) {
				auto balance = accountStateKeyIter.get().Balances.get(mosaicId);
				return balance >= amount;
			}

			return false;
		}
	}

	ValidationResult MosaicLevyTransferValidatorDetail(const model::BalanceTransferNotification<1>& notification,
			const ValidatorContext& context) {

		auto& cache = context.Cache.sub<cache::AccountStateCache>();
		auto& mosaicCache = context.Cache.sub<cache::MosaicCache>();
		
		auto mosaicId = context.Resolvers.resolve(notification.MosaicId);
		auto mosaicIter = mosaicCache.find(mosaicId);
		if (!mosaicIter.tryGet()) {
			return Failure_Mosaic_Id_Not_Found;
		}

		auto& entry = mosaicIter.get();
		auto& levy = entry.levy();

		if(model::UnsetMosaicId == levy.MosaicId || levy.MosaicId.unwrap() == notification.MosaicId.unwrap()){
			/// we are working in  he same mosaic space
			/// balance checking are handled by core system validators
			return ValidationResult::Success;
		}
		
		/// get balance of sender for specific mosaic
		auto senderIter = cache.find(notification.Sender);
		auto recipientIter = cache.find(context.Resolvers.resolve(levy.Recipient));

		if( !recipientIter.tryGet()) {
			return Failure_Mosaic_Recipient_Levy_Not_Exist;
		}

		/// compute levy amount to be deducted
		utils::MosaicLevyCalculatorFactory factory;
		auto result = factory.getCalculator(levy.Type)(notification.Amount, levy.Fee);

		return IsBalanceEnough(cache, notification.Sender, levy.MosaicId, result.levyAmount)
		       ? ValidationResult::Success
		       : Failure_Mosaic_Insufficient_Levy_Balance;
	}

	DECLARE_STATEFUL_VALIDATOR(MosaicLevyTransfer, Notification)( ) {
		return MAKE_STATEFUL_VALIDATOR(MosaicLevyTransfer, [](const auto& notification, const auto& context) {
				return MosaicLevyTransferValidatorDetail(notification, context);
		});
	}
}}
