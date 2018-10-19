/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "AccountStateCache.h"
#include "BalanceView.h"
#include "catapult/model/Address.h"

namespace catapult { namespace cache {
	bool BalanceView::canHarvest(const Key& publicKey, const Height& height, Amount minHarvestingBalance) const {
		return getEffectiveBalance(publicKey, height) >= minHarvestingBalance;
	}

	Amount BalanceView::getEffectiveBalance(const Key& publicKey, const Height& height) const {
		auto pAccountState = m_cache.tryGet(publicKey);

		// if state could not be accessed by public key, try searching by address
		if (!pAccountState)
			pAccountState = m_cache.tryGet(model::PublicKeyToAddress(publicKey, m_cache.networkIdentifier()));

		return pAccountState ? pAccountState->Balances.getEffectiveBalance(height, m_cache.effectiveBalanceRange()) : Amount();
	}
}}
