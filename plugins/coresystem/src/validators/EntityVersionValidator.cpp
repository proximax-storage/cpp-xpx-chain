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

#include <src/catapult/validators/ValidatorContext.h>
#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::EntityNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(EntityVersion, Notification)(const std::shared_ptr<config::LocalNodeConfigurationHolder>& pConfigHolder) {
		return MAKE_STATEFUL_VALIDATOR(EntityVersion, [pConfigHolder](const auto& notification, const auto& context) {
			auto entityIter = pConfigHolder->Config(context.Height).SupportedEntityVersions.find(notification.EntityType);

			if (entityIter == pConfigHolder->Config(context.Height).SupportedEntityVersions.end()) {
				return Failure_Core_Invalid_Version;
			}
			auto iter = entityIter->second.find(notification.EntityVersion);
			return (iter == entityIter->second.end())
				   ? Failure_Core_Invalid_Version
				   : ValidationResult::Success;
		});
	}
}}
