/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/state/StorageStateImpl.h"
#include <boost/multiprecision/cpp_int.hpp>
#include <catapult/utils/StorageUtils.h>
#include "src/utils/Queue.h"

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<1>;
	using BigUint = boost::multiprecision::uint256_t;

	DECLARE_OBSERVER(PeriodicDownloadChannelPayment, Notification)() {
		return MAKE_OBSERVER(PeriodicDownloadChannelPayment, Notification, ([](const Notification& notification, ObserverContext& context) {
			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
			if (!pluginConfig.Enabled || context.Height < Height(2))
				return;

			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (StartDriveVerification)");

			auto& queueCache = context.Cache.template sub<cache::QueueCache>();
			auto& downloadCache = context.Cache.template sub<cache::DownloadChannelCache>();

			utils::QueueAdapter<cache::DownloadChannelCache> queueAdapter(queueCache, state::DownloadChannelPaymentQueueKey, downloadCache);

			if (queueAdapter.isEmpty()) {
				return;
			}

			auto paymentInterval = pluginConfig.DownloadBillingPeriod.seconds();

			// Creating unique eventHash for the observer
			auto eventHash = utils::getDownloadPaymentEventHash(notification.Timestamp, context.Config.Immutable.GenerationHash);

			auto maxIterations = queueAdapter.size();
			for (int i = 0; i < maxIterations; i++) {
				auto& downloadEntry = downloadCache.find(queueAdapter.front().array()).get();

				auto timeSinceLastPayment = (notification.Timestamp - downloadEntry.getLastDownloadApprovalInitiated()).unwrap() / 1000;
				if (timeSinceLastPayment < paymentInterval) {
					break;
				}

				// Pop element from the Queue
				queueAdapter.popFront();

				if (downloadEntry.isCloseInitiated()) {
					continue;
				}
				downloadEntry.decrementDownloadApprovalCount();
				downloadEntry.setLastDownloadApprovalInitiated(notification.Timestamp);
				downloadEntry.downloadApprovalInitiationEvent() = eventHash;

				if (downloadEntry.downloadApprovalCountLeft() > 0) {
					// Channel Continues To Exist
					queueAdapter.pushBack(downloadEntry.entryKey());
				}
				else {
					downloadEntry.setFinishPublished(true);
				}
			}
        }))
	};
}}