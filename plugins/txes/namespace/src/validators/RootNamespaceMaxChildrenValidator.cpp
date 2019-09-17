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
#include "src/config/NamespaceConfiguration.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::ChildNamespaceNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(RootNamespaceMaxChildren, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return MAKE_STATEFUL_VALIDATOR(RootNamespaceMaxChildren, ([pConfigHolder](
				const auto& notification,
				const ValidatorContext& context) {
			const auto& cache = context.Cache.sub<cache::NamespaceCache>();
			auto namespaceIter = cache.find(notification.ParentId);
			const auto& parentEntry = namespaceIter.get();
			const model::NetworkConfiguration& networkConfig = pConfigHolder->Config(context.Height).Network;
			const auto& pluginConfig = networkConfig.GetPluginConfiguration<config::NamespaceConfiguration>(PLUGIN_NAME_HASH(namespace));
			return pluginConfig.MaxChildNamespaces <= parentEntry.root().size() ? Failure_Namespace_Max_Children_Exceeded : ValidationResult::Success;
		}));
	}
}}
