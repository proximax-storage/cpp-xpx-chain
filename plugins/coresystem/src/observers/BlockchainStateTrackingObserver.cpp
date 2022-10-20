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

#include "Observers.h"

namespace catapult { namespace observers {

	namespace {
		void ObserveNotification(const model::BlockNotification<1>& notification, ObserverContext& context) {
			if(!context.Config.Network.EnableStateTracking || NotifyMode::Commit != context.Mode || context.StateChangeFlags == model::StateChangeFlags::None)
				return;
			model::GlobalStateChangeReceipt receipt(model::Receipt_Type_Blockchain_State_Tracking, context.StateChangeFlags);
			context.StatementBuilder().addBlockchainStateReceipt(receipt);
		}
	}
	DEFINE_OBSERVER(BlockchainStateTracking, model::BlockNotification<1>, ObserveNotification);
}}
