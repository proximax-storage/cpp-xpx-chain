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

#include "src/catapult/validators/StatelessValidatorContext.h"
#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::EntityNotification<1>;

	DEFINE_STATELESS_VALIDATOR(EntityVersion, [](const auto& notification, const StatelessValidatorContext& context) {
		const auto& supportedVersions = context.Config.SupportedEntityVersions;
		auto entityIter = supportedVersions.find(notification.EntityType);

		if (entityIter == supportedVersions.end()) {
			return Failure_Core_Invalid_Version;
		}
		auto iter = entityIter->second.find(notification.EntityVersion);
		return (iter == entityIter->second.end())
			   ? Failure_Core_Invalid_Version
			   : ValidationResult::Success;
	})
}}
