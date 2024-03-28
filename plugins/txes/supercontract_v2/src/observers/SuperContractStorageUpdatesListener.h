/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/observers/StorageUpdatesListener.h"
#include "catapult/state/DriveStateBrowser.h"
#include "catapult/observers/LiquidityProviderExchangeObserver.h"

namespace catapult::observers {

class SuperContractStorageUpdatesListener: public StorageUpdatesListener {
public:

	void onDriveClosed(ObserverContext& context, const Key& driveKey) const override;

	SuperContractStorageUpdatesListener(
			const std::unique_ptr<state::DriveStateBrowser>& driveStateBrowser,
			const std::unique_ptr<observers::LiquidityProviderExchangeObserver>& liquidityProvider);

private:

	const std::unique_ptr<state::DriveStateBrowser>& m_driveStateBrowser;
	const std::unique_ptr<observers::LiquidityProviderExchangeObserver>& m_liquidityProvider;

};

}