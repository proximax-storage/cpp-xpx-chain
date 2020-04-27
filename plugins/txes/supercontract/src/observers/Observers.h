/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/observers/ObserverTypes.h"
#include "src/config/SuperContractConfiguration.h"
#include "src/model/SuperContractNotifications.h"
#include "plugins/txes/aggregate/src/model/AggregateNotifications.h"
#include "plugins/txes/service/src/model/ServiceNotifications.h"

namespace catapult { namespace observers {

	/// Observes changes triggered by deploy notifications.
	DECLARE_OBSERVER(Deploy, model::DeployNotification<1>)();

	/// Observes changes triggered by start execute notifications.
	DECLARE_OBSERVER(StartExecute, model::StartExecuteNotification<1>)();

	/// Observes changes triggered by aggregate cosignatures notifications.
	DECLARE_OBSERVER(EndExecuteCosigners, model::AggregateCosignaturesNotification<2>)();

	/// Observes changes triggered by aggregate transaction hash notifications.
	DECLARE_OBSERVER(AggregateTransactionHash, model::AggregateTransactionHashNotification<1>)();

	/// Observes changes triggered by deactivate notifications.
	DECLARE_OBSERVER(Deactivate, model::DeactivateNotification<1>)();

	/// Observes changes triggered by the end drive notifications.
	DECLARE_OBSERVER(EndDrive, model::EndDriveNotification<1>)();

	/// Observes changes triggered by the end execute notifications.
	DECLARE_OBSERVER(EndExecute, model::EndExecuteNotification<1>)();

	/// Observes changes triggered by block notifications.
	DECLARE_OBSERVER(ExpiredExecution, model::BlockNotification<1>)();

	/// Observes changes triggered by suspend notifications.
	DECLARE_OBSERVER(Suspend, model::SuspendNotification<1>)();

	/// Observes changes triggered by resume notifications.
	DECLARE_OBSERVER(Resume, model::ResumeNotification<1>)();
}}
