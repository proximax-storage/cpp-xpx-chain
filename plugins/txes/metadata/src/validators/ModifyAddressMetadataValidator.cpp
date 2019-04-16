/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/catapult/cache_core/AccountStateCache.h"
#include "catapult/model/Address.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::ModifyAddressMetadataNotification;

	namespace {
		Address CopyToAddress(const UnresolvedAddress& address) {
			Address dest;
			std::memcpy(dest.data(), address.data(), address.size());
			return dest;
		}
		namespace {
			ValidationResult validate(const Notification& notification, const ValidatorContext& context) {
				auto address = model::PublicKeyToAddress(notification.Signer, context.Network.Identifier);

				if (address != CopyToAddress(notification.MetadataId))
					return Failure_Metadata_Address_Modification_Not_Permitted;

				const auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
				auto it = accountStateCache.find(notification.Signer);

				if (!it.tryGet()) {
					auto it = accountStateCache.find(address);

					if (!it.tryGet())
						return Failure_Metadata_Address_Is_Not_Exist;
				}

				return ValidationResult::Success;
			}
		}
	}

	DEFINE_STATEFUL_VALIDATOR(ModifyAddressMetadata, [](const auto& notification, const ValidatorContext& context) {
		return validate(notification, context);
	});
}}
