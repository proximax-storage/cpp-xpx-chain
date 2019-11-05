/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "Observers.h"
#include "src/state/DriveEntry.h"
#include "src/utils/ServiceUtils.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace observers {

    void Transfer(state::AccountState& debitState, state::AccountState& creditState, MosaicId mosaicId, Amount amount, Height height);

    void DrivePayment(state::DriveEntry& driveEntry, const ObserverContext& context, const MosaicId& storageMosaicId);

}}
