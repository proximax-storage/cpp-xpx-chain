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
#include "src/model/NamespaceLifetimeConstraints.h"

namespace catapult { namespace validators {

	using Notification = model::RootNamespaceNotification<1>;

	namespace {
		constexpr Height ToHeight(BlockDuration duration) {
			return Height(duration.unwrap());
		}

		constexpr bool AddOverflows(Height height, BlockDuration duration) {
			return height + ToHeight(duration) < height;
		}

		constexpr Height CalculateMaxLifetimeEnd(Height height, BlockDuration duration) {
			return AddOverflows(height, duration) ? Height(std::numeric_limits<uint64_t>::max()) : height + ToHeight(duration);
		}
	}

	DECLARE_STATEFUL_VALIDATOR(NamespaceDurationOverflow, Notification)() {
		return MAKE_STATEFUL_VALIDATOR(NamespaceDurationOverflow, [](
				const auto& notification,
				const ValidatorContext& context) {
			const auto& cache = context.Cache.sub<cache::NamespaceCache>();
			auto height = context.Height;

			if (AddOverflows(height, notification.Duration))
				return Failure_Namespace_Invalid_Duration;

			if (!cache.contains(notification.NamespaceId))
				return ValidationResult::Success;

			// if grace period after expiration has passed, overflow check above is sufficient
			auto namespaceIter = cache.find(notification.NamespaceId);
			const auto& root = namespaceIter.get().root();
			if (!root.lifetime().isActiveOrGracePeriod(height))
				return ValidationResult::Success;

			if (AddOverflows(root.lifetime().End, notification.Duration))
				return Failure_Namespace_Invalid_Duration;

			auto newLifetimeEnd = root.lifetime().End + ToHeight(notification.Duration);
			model::NamespaceLifetimeConstraints constraints(context.Config.Network);
			auto maxLifetimeEnd = CalculateMaxLifetimeEnd(height, constraints.maxNamespaceDuration());
			if (newLifetimeEnd > maxLifetimeEnd)
				return Failure_Namespace_Invalid_Duration;

			return ValidationResult::Success;
		});
	}
}}
