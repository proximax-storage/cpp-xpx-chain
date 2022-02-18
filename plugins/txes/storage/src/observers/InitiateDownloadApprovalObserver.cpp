/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/state/StorageStateImpl.h"
#include <random>
#include <boost/multiprecision/cpp_int.hpp>

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<2>;
	using BigUint = boost::multiprecision::uint256_t;

	DECLARE_OBSERVER(InitiateDownloadApproval, Notification)(state::StorageStateImpl& state, const cache::DriveKeyCollector& driveKeyCollector) {
		return MAKE_OBSERVER(InitiateDownloadApproval, Notification, ([&state, &driveKeyCollector](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (InitiateDownloadApproval)");

			// TODO: temporary turn off verifications.
			return;

			if (context.Height < Height(2))
				return;

			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();

			auto& downloadCache = context.Cache.template sub<cache::DownloadChannelCache>();

			for (const auto& downloadChannelCache : driveKeyCollector.keys()) {
//				auto deltaHours = (notification.Timestamp - )
			}
        }))
	};
}}