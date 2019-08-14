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
#include "src/cache/MultisigCache.h"
#include "src/config/MultisigConfiguration.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::ModifyMultisigNewCosignerNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(ModifyMultisigMaxCosignedAccounts, Notification)(const std::shared_ptr<config::LocalNodeConfigurationHolder>& pConfigHolder) {
		return MAKE_STATEFUL_VALIDATOR(ModifyMultisigMaxCosignedAccounts, [pConfigHolder](
				const auto& notification,
				const ValidatorContext& context) {
			const auto& multisigCache = context.Cache.sub<cache::MultisigCache>();
			if (!multisigCache.contains(notification.CosignatoryKey))
				return ValidationResult::Success;

			auto multisigIter = multisigCache.find(notification.CosignatoryKey);
			const auto& cosignatoryEntry = multisigIter.get();
			const model::BlockChainConfiguration& blockChainConfig = pConfigHolder->Config(context.Height).BlockChain;
			const auto& pluginConfig = blockChainConfig.GetPluginConfiguration<config::MultisigConfiguration>(PLUGIN_NAME_HASH(multisig));
			return cosignatoryEntry.multisigAccounts().size() >= pluginConfig.MaxCosignedAccountsPerAccount
					? Failure_Multisig_Modify_Max_Cosigned_Accounts
					: ValidationResult::Success;
		});
	}
}}
