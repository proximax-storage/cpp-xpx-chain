/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/model/Address.h"
#include "plugins/txes/lock_secret/src/cache/SecretLockInfoCache.h"
#include "plugins/txes/lock_secret/src/model/LockHashUtils.h"

namespace catapult { namespace validators {

	void VerificationStatus(const state::DriveEntry& driveEntry, const validators::StatefulValidatorContext& context, bool& started, bool& active) {
		const auto& secretLockCache = context.Cache.template sub<cache::SecretLockInfoCache>();
		auto driveAddress = PublicKeyToAddress(driveEntry.key(), context.NetworkIdentifier);
		auto key = model::CalculateSecretLockInfoHash(Hash256(), driveAddress);
		started = secretLockCache.contains(key);
		active = secretLockCache.isActive(key, context.Height);
	}
}}
