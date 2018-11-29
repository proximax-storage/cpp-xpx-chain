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
#include "src/cache/NamespaceCache.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::ChildNamespaceNotification;

	DEFINE_STATEFUL_VALIDATOR(ChildNamespaceAvailability, [](const auto& notification, const ValidatorContext& context) {
		const auto& cache = context.Cache.sub<cache::NamespaceCache>();
		auto height = context.Height;

		if (cache.contains(notification.NamespaceId))
			return Failure_Namespace_Already_Exists;

		if (!cache.contains(notification.ParentId))
			return Failure_Namespace_Parent_Unknown;

		auto namespaceIter = cache.find(notification.ParentId);
		const auto& parentEntry = namespaceIter.get();
		const auto& parentPath = parentEntry.ns().path();
		if (parentPath.size() == parentPath.capacity())
			return Failure_Namespace_Too_Deep;

		const auto& root = parentEntry.root();
		if (!root.lifetime().isActiveAndUnlocked(height))
			return Failure_Namespace_Expired;

		if (root.owner() != notification.Signer)
			return Failure_Namespace_Owner_Conflict;

		return ValidationResult::Success;
	});
}}
