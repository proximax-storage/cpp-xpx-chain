/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/observers/ObserverTypes.h"
#include "src/config/StorageConfiguration.h"
#include "src/model/StorageNotifications.h"
#include "src/state/DownloadChannelEntry.h"
#include "src/cache/DownloadCache.h"

namespace catapult { namespace observers {

	/// Observes changes triggered by download notifications.
	DECLARE_OBSERVER(DownloadChannel, model::DownloadNotification<1>)();
}}
