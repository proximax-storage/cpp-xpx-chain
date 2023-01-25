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
		  	viewSequenceEntry.sequence() = notification.Sequence;
		  	viewSequenceCache.insert(viewSequenceEntry);
		}))
	}
}}
