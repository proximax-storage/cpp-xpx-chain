/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/state/StorageStateImpl.h"
#include <random>
#include <boost/multiprecision/cpp_int.hpp>
#include "Queue.h"

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<2>;
	using BigUint = boost::multiprecision::uint256_t;

	DECLARE_OBSERVER(PeriodicDownloadChannelPayment, Notification)() {
		return MAKE_OBSERVER(PeriodicDownloadChannelPayment, Notification, ([](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (StartDriveVerification)");

			CATAPULT_LOG( error ) << "Verification Observer";

			if (context.Height < Height(2))
				return;

			auto& queueCache = context.Cache.template sub<cache::QueueCache>();
			auto& downloadCache = context.Cache.template sub<cache::DownloadChannelCache>();

			QueueAdapter<cache::DownloadChannelCache> queueAdapter(queueCache, state::DownloadChannelPaymentQueueKey, downloadCache);

			if (queueAdapter.isEmpty()) {
				return;
			}

			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
			auto paymentInterval = pluginConfig.DownloadBillingPeriod.seconds();

			for (int i = 0; i < downloadCache.size(); i++) {
				auto& downloadEntry = downloadCache.find(queueAdapter.front().array()).get();

				auto timeSinceLastPayment = (notification.Timestamp - downloadEntry.getLastDownloadApprovalInitiated()).unwrap() / 1000;
				CATAPULT_LOG( error ) << "time " << timeSinceLastPayment << " " << paymentInterval;
				if (timeSinceLastPayment < paymentInterval) {
					break;
				}

				// Pop element from the Queue
				queueAdapter.popFront();

				if (downloadEntry.isFinishPublished()) {
					continue;
				}

				downloadEntry.decrementDownloadApprovalCount();
				downloadEntry.setLastDownloadApprovalInitiated(notification.Timestamp);
				downloadEntry.downloadApprovalInitiationEvent() = notification.Hash;

				if (downloadEntry.downloadApprovalCountLeft() > 0) {
					// Channel Continues To Exist
					queueAdapter.pushBack(downloadEntry.entryKey());
				}
			}
        }))
	};
}}