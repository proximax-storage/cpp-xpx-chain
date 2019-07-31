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
#include "catapult/model/Address.h"
#include "catapult/model/ResolverContext.h"

namespace catapult { namespace validators {

	using Notification = model::ModifyAddressPropertyValueNotification_v1;

	DECLARE_STATEFUL_VALIDATOR(PropertyAddressNoSelfModification, Notification)(const std::shared_ptr<config::LocalNodeConfigurationHolder>& pConfigHolder) {
		return MAKE_STATEFUL_VALIDATOR(PropertyAddressNoSelfModification, [pConfigHolder](const auto& notification, const auto& context) {
			const model::BlockChainConfiguration& config = pConfigHolder->Config(context.Height).BlockChain;
			auto address = model::PublicKeyToAddress(notification.Key, config.Network.Identifier);
			return address != model::ResolverContext().resolve(notification.Modification.Value)
					? ValidationResult::Success
					: Failure_Property_Modification_Address_Invalid;
		});
	}
}}
