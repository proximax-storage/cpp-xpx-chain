/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/LevyCache.h"
#include "src/model/MosaicLevy.h"
#include "src/utils/MosaicLevyUtils.h"

namespace catapult { namespace validators {
		
	using Notification = model::MosaicRemoveLevyNotification<1>;

	ValidationResult MosaicRemoveLevyValidatorDetail(const Notification& notification,
													 const ValidatorContext& context) {
		/// 1. check if signer is eligible and mosaic ID to be removed is valid
		/// allow the nemesis signer to remove a levy from cache even if the base mosaic Id expired
		/// mosaic owner cannot be checked if mosaic expired and no longer in cache
		auto mosaicId = context.Resolvers.resolve(notification.MosaicId);
		auto result = utils::CheckLevyOperationAllowed(notification.Signer, mosaicId, context);
		if (result != ValidationResult::Success && notification.Signer != context.Network.PublicKey)
			return result;

		/// 2. Check if levy for this mosaicID exist
		auto &levyCache = context.Cache.sub<cache::LevyCache>();
		auto iter = levyCache.find(mosaicId);
		if (!iter.tryGet())
			return Failure_Mosaic_Levy_Entry_Not_Found;

		/// 3. Check if current levy is set
		auto entry = iter.get();
		if (entry.levy() == nullptr)
			return Failure_Mosaic_Current_Levy_Not_Set;

		return ValidationResult::Success;
	}
	
	DECLARE_STATEFUL_VALIDATOR(RemoveLevy, Notification)( ) {
		return MAKE_STATEFUL_VALIDATOR(RemoveLevy, [](const auto& notification, const auto& context) {
			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::MosaicConfiguration>();
			if (!pluginConfig.LevyEnabled)
				return Failure_Mosaic_Levy_Not_Enabled;

			return MosaicRemoveLevyValidatorDetail(notification, context);
		});
	}
}}
