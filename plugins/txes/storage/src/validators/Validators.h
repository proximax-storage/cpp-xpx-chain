/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"
#include "catapult/model/StorageNotifications.h"
#include "src/cache/BcDriveCache.h"
#include "src/cache/ReplicatorCache.h"

namespace catapult { namespace validators {

	void VerificationStatus(const state::BcDriveEntry& driveEntry, const validators::ValidatorContext& context, bool& started, bool& active);

	/// A validator implementation that applies to drive prepare drive notifications and validates that:
	/// - drive size >= minDriveSize
	/// - number of replicators >= minReplicatorCount
	/// - the drive does not exist
	DECLARE_STATEFUL_VALIDATOR(PrepareDrive, model::PrepareDriveNotification<1>)(const std::shared_ptr<cache::ReplicatorKeyCollector>& pKeyCollector);

	/// A validator implementation that applies to drive data modification cancel notifications and validates that:
	/// -
	DECLARE_STATEFUL_VALIDATOR(DataModification, model::DataModificationNotification<1>)();

	/// A validator implementation that applies to drive data modification approval notifications and validates that:
	/// - respective data modification is present in activeDataModifications
	/// - respective data modification is the first (oldest) element in activeDataModifications
	DECLARE_STATEFUL_VALIDATOR(DataModificationApproval, model::DataModificationApprovalNotification<1>)();

	/// A validator implementation that applies to drive data modification cancel notifications and validates that:
	/// -
	DECLARE_STATEFUL_VALIDATOR(DataModificationCancel, model::DataModificationCancelNotification<1>)();

	/// A validator implementation that applies to drive data modification cancel notifications and validates that:
	/// -
	DECLARE_STATEFUL_VALIDATOR(ReplicatorOnboarding, model::ReplicatorOnboardingNotification<1>)();

	/// A validator implementation that applies to drive closure notifications and validates that:
	/// -
	DECLARE_STATEFUL_VALIDATOR(DriveClosure, model::DriveClosureNotification<1>)();

}}
