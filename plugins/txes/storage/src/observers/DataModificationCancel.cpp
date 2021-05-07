/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(
			DataModificationCancel,
			model::DataModificationCancelNotification<1>,
			[](const model::DataModificationCancelNotification<1>& notification, ObserverContext& context) {
				if (NotifyMode::Commit != context.Mode) {
					return;
				}

				auto& driveCache = context.Cache.sub<cache::DriveCache>();
				auto& driveEntry = driveCache.find(notification.DriveKey).get();
				driveEntry.dataModificationQueue().erase(std::find_if(
						driveEntry.dataModificationQueue().begin(),
						driveEntry.dataModificationQueue().end(),
						[&notification](const auto& element) {
							return element.first == notification.ModificationTrx;
						}));
			})
}}
