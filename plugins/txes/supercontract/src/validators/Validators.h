/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"
#include "src/model/SuperContractNotifications.h"
#include "src/state/SuperContractEntry.h"
#include "plugins/txes/service/src/model/ServiceNotifications.h"

namespace catapult { namespace validators {

	/// A validator check that drive is not finished
	DECLARE_STATEFUL_VALIDATOR(Drive, model::DriveNotification<1>)();

	/// A validator check that deploy transaction is valid
	DECLARE_STATEFUL_VALIDATOR(Deploy, model::DeployNotification<1>)();

	/// TODO: During implementation of deactivating super contract also remember about DriveFinishNotification
	/// A validator check that drive file system transaction doesn't remove super contract file
	DECLARE_STATEFUL_VALIDATOR(DriveFileSystem, model::DriveFileSystemNotification<1>)();
}}
