/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/observers/ObserverTypes.h"
#include "src/config/DbrbConfiguration.h"
#include "src/model/DbrbNotifications.h"

namespace catapult {
	namespace cache { class DbrbViewFetcherImpl; }
	namespace observers { class DbrbProcessUpdateListener; }
	namespace plugins { class PluginManager; }
}

namespace catapult { namespace observers {

		/// Observes changes triggered by add DBRB process notifications.
		DECLARE_OBSERVER(AddDbrbProcess, model::AddDbrbProcessNotification<1>)();

		/// Observes changes triggered by block notifications.
		DECLARE_OBSERVER(DbrbProcessPruning, model::BlockNotification<1>)(const std::vector<std::unique_ptr<DbrbProcessUpdateListener>>& updateListeners);

		/// Observes changes triggered by remove DBRB process notifications.
		DECLARE_OBSERVER(RemoveDbrbProcess, model::RemoveDbrbProcessNotification<1>)();

		/// Observes changes triggered by block notifications.
		DECLARE_OBSERVER(DbrbProcessUpdate, model::BlockNotification<1>)(const plugins::PluginManager& pluginManager);
}}
