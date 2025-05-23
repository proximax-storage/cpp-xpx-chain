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
#include "src/model/AliasNotifications.h"
#include "src/model/NamespaceNotifications.h"
#include "catapult/model/Notifications.h"
#include "catapult/observers/ObserverTypes.h"

namespace catapult { namespace observers {

	// region alias

	/// Observes changes triggered by aliased address notifications, including:
	/// - linking/unlinking address to namespace
	DECLARE_OBSERVER(AliasedAddress, model::AliasedAddressNotification_v1)();

	/// Observes changes triggered by aliased mosaic id notifications, including:
	/// - linking/unlinking mosaic id to namespace
	DECLARE_OBSERVER(AliasedMosaicId, model::AliasedMosaicIdNotification_v1)();

	// endregion

	// region namespace

	/// Observes changes triggered by root namespace notifications, including:
	/// - creation of namespaces
	DECLARE_OBSERVER(RootNamespace, model::RootNamespaceNotification<1>)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	/// Observes changes triggered by child namespace notifications, including:
	/// - creation of namespaces
	DECLARE_OBSERVER(ChildNamespace, model::ChildNamespaceNotification<1>)();

	// endregion
}}
