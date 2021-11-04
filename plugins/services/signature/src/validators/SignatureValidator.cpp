/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include <catapult/cache_core/AccountStateCache.h>
#include "Validators.h"
#include "catapult/crypto/Signer.h"
#include "catapult/utils/SignatureVersionToKeyTypeResolver.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/utils/CacheUtils.h"

namespace catapult { namespace validators {


	DECLARE_STATELESS_VALIDATOR(SignatureV1, model::SignatureNotification<1>)(const GenerationHash& generationHash) {
		return MAKE_STATELESS_VALIDATOR_WITH_TYPE(SignatureV1, model::SignatureNotification<1>, [generationHash](const auto& notification) {

			auto isVerified = model::SignatureNotification<1>::ReplayProtectionMode::Enabled == notification.DataReplayProtectionMode
					? crypto::Verify(notification.Signer, { generationHash, notification.Data }, notification.Signature, utils::ResolveKeyHashingTypeFromSignatureVersion(1))
					: crypto::Verify(notification.Signer, {notification.Data}, notification.Signature, utils::ResolveKeyHashingTypeFromSignatureVersion(1));

			return isVerified ? ValidationResult::Success : Failure_Signature_Not_Verifiable;
		});
	}
	DECLARE_STATEFUL_VALIDATOR(SignatureV2, model::SignatureNotification<2>)(const GenerationHash& generationHash) {
		return MAKE_STATEFUL_VALIDATOR_WITH_TYPE(SignatureV2, model::SignatureNotification<2>, ([generationHash](const auto& notification, const auto& context) {

			const auto& cache = context.Cache.template sub<cache::AccountStateCache>();
			auto account = utils::FindAccountStateByPublicKeyOrAddress(cache, notification.Signer);
			if(account)
			{
				/// Locked accounts can no longer sign
				if(account->get().IsLocked())
					return Failure_Signature_Not_Verifiable;
				if(!utils::VerifyAccountVersionCompatibilityWithSignatureVersion(account->get().GetVersion(), notification.SignatureVersion)) return Failure_Signature_Invalid_Version;
			}
			else if(utils::ResolveAccountVersionToSignatureVersion(context.Config.Network.AccountVersion) > notification.SignatureVersion)
			{
				//Make sure that accounts cannot be created if they are using a signature scheme older than the current active one
				return Failure_Signature_Invalid_Version;
			}
			auto isVerified = model::SignatureNotification<1>::ReplayProtectionMode::Enabled == notification.DataReplayProtectionMode
						? crypto::Verify(notification.Signer, { generationHash, notification.Data }, notification.Signature, utils::ResolveKeyHashingTypeFromSignatureVersion(notification.SignatureVersion))
						: crypto::Verify(notification.Signer, {notification.Data}, notification.Signature, utils::ResolveKeyHashingTypeFromSignatureVersion(notification.SignatureVersion));

			return isVerified ? ValidationResult::Success : Failure_Signature_Not_Verifiable;
		}));
	}
}}
