/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"
#include "src/model/StorageNotifications.h"
#include "src/state/DriveEntry.h"
#include "src/state/DownloadChannelEntry.h"
#include "plugins/txes/aggregate/src/model/AggregateNotifications.h"

namespace catapult { namespace validators {

	void VerificationStatus(const state::DriveEntry& driveEntry, const validators::ValidatorContext& context, bool& started, bool& active);

	/// A validator implementation that applies to drive prepare drive notifications and validates that:
	/// - drive size >= minDriveSize
	/// - number of replicators >= minReplicatorCount
	DECLARE_STATEFUL_VALIDATOR(PrepareDrivePermission, model::PrepareDriveNotification<1>)();

	/// A validator implementation that applies to drive data modification approval notifications and validates that:
	/// - respective data modification is present in activeDataModifications
	/// - respective data modification is the first (oldest) element in activeDataModifications
	DECLARE_STATEFUL_VALIDATOR(DataModificationApproval, model::DataModificationApprovalNotification<1>)();

}}
