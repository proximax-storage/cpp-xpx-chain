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

#pragma once
#include "Results.h"
#include "src/model/SecretLockNotifications.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {

	/// A validator implementation that applies to secret lock notifications and validates that:
	/// - lock duration is at most \a maxSecretLockDuration
	DECLARE_STATEFUL_VALIDATOR(SecretLockDuration, model::SecretLockDurationNotification<1>)();

	/// A validator implementation that applies to secret lock hash algorithm notifications and validates that:
	/// - hash algorithm is valid
	DECLARE_STATELESS_VALIDATOR(SecretLockHashAlgorithm, model::SecretLockHashAlgorithmNotification<1>)();

	/// A validator implementation that applies to secret lock notifications and validates that:
	/// - attached hash is not present in secret lock info cache
	DECLARE_STATEFUL_VALIDATOR(SecretLockCacheUnique, model::SecretLockNotification<1>)();

	/// A validator implementation that applies to proof notifications and validates that:
	/// - hash algorithm is supported
	/// - proof size is within inclusive bounds of \a minProofSize and \a maxProofSize
	/// - hash of proof matches secret
	DECLARE_STATEFUL_VALIDATOR(ProofSecret, model::ProofSecretNotification<1>)();

	/// A validator implementation that applies to proof notifications and validates that:
	/// - secret obtained from proof is present in cache
	DECLARE_STATEFUL_VALIDATOR(Proof, model::ProofPublicationNotification<1>)();

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(SecretLockPluginConfig, model::PluginConfigNotification<1>)();
}}
