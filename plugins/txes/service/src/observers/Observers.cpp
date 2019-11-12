/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	void SetDriveState(state::DriveEntry& entry, ObserverContext& context, state::DriveState driveState) {
		if (NotifyMode::Commit == context.Mode)
			context.StatementBuilder().addPublicKeyReceipt(model::DriveStateReceipt(model::Receipt_Type_Drive_State, entry.key(), utils::to_underlying_type(driveState)));
		entry.setState(driveState);
	}
}}
