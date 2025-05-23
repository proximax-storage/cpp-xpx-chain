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
#include "src/model/MosaicNotifications.h"
#include "catapult/model/Notifications.h"
#include "catapult/observers/ObserverTypes.h"

namespace catapult { namespace observers {

	/// Observes changes triggered by mosaic definition notifications, including:
	/// - creation of mosaics
	DECLARE_OBSERVER(MosaicDefinition, model::MosaicDefinitionNotification<1>)();

	/// Observes changes triggered by mosaic supply change notifications, including:
	/// - increase or decrease of supply
	DECLARE_OBSERVER(MosaicSupplyChange, model::MosaicSupplyChangeNotification<1>)();
	
	/// Observer triggered during addition of levy
	DECLARE_OBSERVER(ModifyLevy, model::MosaicModifyLevyNotification<1>)();
	
	/// Observes removal of levy
	DECLARE_OBSERVER(RemoveLevy, model::MosaicRemoveLevyNotification<1>)();
		
	/// Observes changes triggered by block notifications
	DECLARE_OBSERVER(PruneLevyHistory, model::BlockNotification<1>)();
	
	/// Observes balance changes triggered by balance transfer notifications.
	DECLARE_OBSERVER(LevyBalanceTransfer, model::BalanceTransferNotification<1>)();
}}
