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

#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/crypto/Signature.h"
#include "Validators.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/cache_core/AccountStateCacheUtils.h"

namespace catapult { namespace validators {

	namespace {



		template<bool isBlock, typename TNotification>
		auto Validate(const TNotification& notification, const ValidatorContext& context, const GenerationHash& generationHash)
		{
			const auto& cache = context.Cache.template sub<cache::AccountStateCache>();
			auto account = cache::FindAccountStateByPublicKeyOrAddress(cache, notification.Signer);
			if(account)
			{
				/// Locked accounts can no longer sign
				if(account->IsLocked())
					return VerifiableFailureResolver<isBlock, Failure_Signature_Block_Not_Verifiable, Failure_Signature_Not_Verifiable>::ValidationResult;
				if(!utils::AccountVersionFeatureResolver::IsAccountShemeCompatible(account->GetVersion(), notification.DerivationScheme)) return VerifiableFailureResolver<isBlock, Failure_Signature_Block_Invalid_Version, Failure_Signature_Invalid_Version>::ValidationResult;
			}
			// If
			if(!utils::AccountVersionFeatureResolver::MinimumVersionHasCompatibleDerivationScheme(notification.DerivationScheme, context.Config.Network.MinimumAccountVersion, context.Config.Network.AccountVersion))
			{
				return VerifiableFailureResolver<isBlock, Failure_Signature_Block_Invalid_Version, Failure_Signature_Invalid_Version>::ValidationResult;
			}
			auto isVerified = model::Replay::ReplayProtectionMode::Enabled == notification.DataReplayProtectionMode
									  ? crypto::SignatureFeatureSolver::Verify(notification.Signer, { generationHash, notification.Data }, notification.Signature, notification.DerivationScheme)
									  : crypto::SignatureFeatureSolver::Verify(notification.Signer, {notification.Data}, notification.Signature, notification.DerivationScheme);

			return isVerified ? ValidationResult::Success : VerifiableFailureResolver<isBlock, Failure_Signature_Block_Not_Verifiable, Failure_Signature_Not_Verifiable>::ValidationResult;

		}
	}

	DECLARE_STATELESS_VALIDATOR(SignatureV1, model::SignatureNotification<1>)(const GenerationHash& generationHash) {
		return MAKE_STATELESS_VALIDATOR_WITH_TYPE(SignatureV1, model::SignatureNotification<1>, [generationHash](const auto& notification) {

			auto isVerified = model::Replay::ReplayProtectionMode::Enabled == notification.DataReplayProtectionMode
					? crypto::SignatureFeatureSolver::Verify(notification.Signer, { generationHash, notification.Data }, notification.Signature, DerivationScheme::Ed25519_Sha3)
					: crypto::SignatureFeatureSolver::Verify(notification.Signer, {notification.Data}, notification.Signature, DerivationScheme::Ed25519_Sha3);

			return isVerified ? ValidationResult::Success : Failure_Signature_Not_Verifiable;
		});
	}
	DECLARE_STATELESS_VALIDATOR(BlockSignatureV1, model::BlockSignatureNotification<1>)(const GenerationHash& generationHash) {
		return MAKE_STATELESS_VALIDATOR_WITH_TYPE(BlockSignatureV1, model::BlockSignatureNotification<1>, [generationHash](const auto& notification) {

			auto isVerified = model::Replay::ReplayProtectionMode::Enabled == notification.DataReplayProtectionMode
									  ? crypto::SignatureFeatureSolver::Verify(notification.Signer, { generationHash, notification.Data }, notification.Signature, DerivationScheme::Ed25519_Sha3)
									  : crypto::SignatureFeatureSolver::Verify(notification.Signer, {notification.Data}, notification.Signature, DerivationScheme::Ed25519_Sha3);

			return isVerified ? ValidationResult::Success : Failure_Signature_Block_Not_Verifiable;
		});
	}
	DECLARE_STATEFUL_VALIDATOR(SignatureV2, model::SignatureNotification<2>)(const GenerationHash& generationHash) {
		return MAKE_STATEFUL_VALIDATOR_WITH_TYPE(SignatureV2, model::SignatureNotification<2>, ([generationHash](const auto& notification, const auto& context) {
		  return Validate<false>(notification, context, generationHash);

		}));
	}
	DECLARE_STATEFUL_VALIDATOR(BlockSignatureV2, model::BlockSignatureNotification<2>)(const GenerationHash& generationHash) {
		return MAKE_STATEFUL_VALIDATOR_WITH_TYPE(BlockSignatureV2, model::BlockSignatureNotification<2>, ([generationHash](const auto& notification, const auto& context) {
			 return Validate<true>(notification, context, generationHash);
		 }));
	}
}}
