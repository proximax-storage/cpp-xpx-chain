/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "Observers.h"
#include "src/cache/DriveCache.h"
#include "src/utils/ServiceUtils.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace observers {

    void Transfer(state::AccountState& debitState, state::AccountState& creditState, MosaicId mosaicId, Amount amount, ObserverContext& context);

    void Credit(state::AccountState& creditState, MosaicId mosaicId, Amount amount, ObserverContext& context);

    void Debit(state::AccountState& debitState, MosaicId mosaicId, Amount amount, ObserverContext& context);

    void DrivePayment(state::DriveEntry& driveEntry, ObserverContext& context, const MosaicId& storageMosaicId, std::vector<Key> replicators);

	void SetDriveState(state::DriveEntry& entry, ObserverContext& context, state::DriveState driveState);

	void UpdateDriveMultisigSettings(state::DriveEntry& entry, ObserverContext& context);

}}
