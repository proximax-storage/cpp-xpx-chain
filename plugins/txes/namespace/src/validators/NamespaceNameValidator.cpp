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
#include "src/config/NamespaceConfiguration.h"
#include "src/model/NameChecker.h"
#include "src/model/NamespaceIdGenerator.h"

namespace catapult { namespace validators {

	using Notification = model::NamespaceNameNotification<1>;
	using NameSet = std::unordered_set<std::string>;

	DECLARE_STATEFUL_VALIDATOR(NamespaceName, Notification)() {
		return MAKE_STATEFUL_VALIDATOR(NamespaceName, ([](const auto& notification, const auto& context) {
			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::NamespaceConfiguration>();
			const auto& networkConfig = context.Config.Network;
			std::unordered_set<NamespaceId, utils::BaseValueHasher<NamespaceId>> reservedRootIds;
			for (const auto& name : pluginConfig.ReservedRootNamespaceNames)
				reservedRootIds.emplace(model::GenerateNamespaceId(Namespace_Base_Id, name));

			if (pluginConfig.MaxNameSize < notification.NameSize || !model::IsValidName(notification.NamePtr, notification.NameSize))
				return Failure_Namespace_Invalid_Name;

			auto name = utils::RawString(reinterpret_cast<const char*>(notification.NamePtr), notification.NameSize);
			if (notification.NamespaceId != model::GenerateNamespaceId(notification.ParentId, name))
				return Failure_Namespace_Name_Id_Mismatch;

			auto namespaceId = Namespace_Base_Id == notification.ParentId ? notification.NamespaceId : notification.ParentId;
			if (reservedRootIds.cend() != reservedRootIds.find(namespaceId) && notification.Signer != networkConfig.Info.PublicKey)
				return Failure_Namespace_Root_Name_Reserved;

			return ValidationResult::Success;
		}));
	}
}}
