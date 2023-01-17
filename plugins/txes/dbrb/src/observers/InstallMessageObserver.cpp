/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::InstallMessageNotification<1>;

	DECLARE_OBSERVER(InstallMessage, Notification)() {
		return MAKE_OBSERVER(PrepareDrive, Notification, ([](const Notification& notification, const ObserverContext& context) {
		  	if (NotifyMode::Rollback == context.Mode)
			  	CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (InstallMessage)");

		  	auto& viewSequenceCache = context.Cache.sub<cache::ViewSequenceCache>();
		  	state::ViewSequenceEntry viewSequenceEntry(notification.MessageHash);

			// Forming the converged sequence
			dbrb::Sequence convergedSequence;

		  	auto pProcessId = notification.ViewProcessIdsPtr;
		  	auto pMembershipChange = notification.MembershipChangesPtr;
		  	auto pViewSize = notification.ViewSizesPtr + 1;	// Skipping the size of the replaced view

		  	dbrb::View view;
		  	auto& viewData = view.Data;
		  	for (auto i = 0u; i < notification.MostRecentViewSize; ++i, ++pProcessId, ++pMembershipChange) {
			  	dbrb::ProcessId processId = *pProcessId;
			  	const auto membershipChange = static_cast<const dbrb::MembershipChange>(*pMembershipChange);
			  	viewData.emplace(processId, membershipChange);

			  	if (viewData.size() >= *pViewSize) {
					convergedSequence.tryAppend(view);
				  	++pViewSize;
				}
		  	}

		  	viewSequenceEntry.sequence() = convergedSequence;
		  	viewSequenceCache.insert(viewSequenceEntry);

		  	// TODO: Update MessageHash entry to have value of notification.MessageHash
		}))
	}
}}
