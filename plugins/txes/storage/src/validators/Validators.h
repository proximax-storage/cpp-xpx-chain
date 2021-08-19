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

	/// A validator implementation that applies to drive finish download notifications and validates that:
	/// - respective download channel exists
	/// - transaction signer is the owner of the respective download channel
	DECLARE_STATEFUL_VALIDATOR(FinishDownload, model::FinishDownloadNotification<1>)();

	/// A validator implementation that applies to drive download payment notifications and validates that:
	/// - respective download channel exists
	DECLARE_STATEFUL_VALIDATOR(DownloadPayment, model::DownloadPaymentNotification<1>)();

	/// A validator implementation that applies to drive storage payment notifications and validates that:
	/// - respective drive exists
	DECLARE_STATEFUL_VALIDATOR(StoragePayment, model::StoragePaymentNotification<1>)();

	/// A validator implementation that applies to drive data modification single approval notifications and validates that:
	/// - respective drive exists
	/// - transaction signer is a replicator of the drive
	/// - respective data modification is the last (newest) among approved data modifications
	/// - percents in upload opinion sum up to 100
	/// - each key in upload opinion is either a key of one of the current replicators of the drive, or a key of the drive owner
	/// - each key in upload opinion appears exactly once
	DECLARE_STATEFUL_VALIDATOR(DataModificationSingleApproval, model::DataModificationSingleApprovalNotification<1>)();

	/// A validator implementation that applies to drive verification payment notifications and validates that:
	/// - respective drive exists
	/// - transaction signer is the owner of the respective drive
	DECLARE_STATEFUL_VALIDATOR(VerificationPayment, model::VerificationPaymentNotification<1>)();
}}
