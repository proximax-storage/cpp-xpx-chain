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

#include "AccountStateCacheView.h"
#include "catapult/model/Address.h"
#include "catapult/model/NetworkInfo.h"

namespace catapult { namespace cache {

	BasicAccountStateCacheView::BasicAccountStateCacheView(
			const AccountStateCacheTypes::BaseSets& accountStateSets,
			const AccountStateCacheTypes::Options& options,
			const model::AddressSet& highValueAddresses)
			: BasicAccountStateCacheView(
					accountStateSets,
					options,
					highValueAddresses,
					std::make_unique<AccountStateCacheViewMixins::KeyLookupAdapter>(
							accountStateSets.KeyLookupMap,
							accountStateSets.Primary))
	{}

	BasicAccountStateCacheView::BasicAccountStateCacheView(
			const AccountStateCacheTypes::BaseSets& accountStateSets,
			const AccountStateCacheTypes::Options& options,
			const model::AddressSet& highValueAddresses,
			std::unique_ptr<AccountStateCacheViewMixins::KeyLookupAdapter>&& pKeyLookupAdapter)
			: AccountStateCacheViewMixins::Size(accountStateSets.Primary)
			, AccountStateCacheViewMixins::ContainsAddress(accountStateSets.Primary)
			, AccountStateCacheViewMixins::ContainsKey(accountStateSets.KeyLookupMap)
			, AccountStateCacheViewMixins::Iteration(accountStateSets.Primary)
			, AccountStateCacheViewMixins::ConstAccessorAddress(accountStateSets.Primary)
			, AccountStateCacheViewMixins::ConstAccessorKey(*pKeyLookupAdapter)
			, m_networkIdentifier(options.NetworkIdentifier)
			, m_importanceGrouping(options.ImportanceGrouping)
			, m_highValueAddresses(highValueAddresses)
			, m_pKeyLookupAdapter(std::move(pKeyLookupAdapter))
	{}

	model::NetworkIdentifier BasicAccountStateCacheView::networkIdentifier() const {
		return m_networkIdentifier;
	}

	uint64_t BasicAccountStateCacheView::importanceGrouping() const {
		return m_importanceGrouping;
	}

	size_t BasicAccountStateCacheView::highValueAddressesSize() const {
		return m_highValueAddresses.size();
	}
}}
