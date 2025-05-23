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

#include "Validators.h"
#include "src/config/SecretLockConfiguration.h"
#include "src/model/LockHashUtils.h"
#include "catapult/validators/ValidatorUtils.h"

namespace catapult { namespace validators {

	using Notification = model::ProofSecretNotification<1>;

	namespace {
		constexpr bool SupportedHash(model::LockHashAlgorithm hashAlgorithm) {
			return ValidationResult::Success == ValidateLessThanOrEqual(
					hashAlgorithm,
					model::LockHashAlgorithm::Op_Hash_256,
					ValidationResult::Failure);
		}
	}

	DECLARE_STATEFUL_VALIDATOR(ProofSecret, Notification)() {
		return MAKE_STATEFUL_VALIDATOR(ProofSecret, [](const auto& notification, const auto& context) {
			if (!SupportedHash(notification.HashAlgorithm))
				return Failure_LockSecret_Hash_Not_Implemented;

			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::SecretLockConfiguration>();
			if (notification.Proof.Size < pluginConfig.MinProofSize || notification.Proof.Size > pluginConfig.MaxProofSize)
				return Failure_LockSecret_Proof_Size_Out_Of_Bounds;

			auto secret = model::CalculateHash(notification.HashAlgorithm, notification.Proof);
			return notification.Secret == secret ? ValidationResult::Success : Failure_LockSecret_Secret_Mismatch;
		});
	}
}}
