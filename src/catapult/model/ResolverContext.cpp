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

#include "ResolverContext.h"
#include <cstring>

namespace catapult { namespace model {

	ResolverContext::ResolverContext()
			: ResolverContext(
					[](auto mosaicId) { return MosaicId(mosaicId.unwrap()); },
					[](const auto& address) {
						Address resolvedAddress;
						std::memcpy(resolvedAddress.data(), address.data(), address.size());
						return resolvedAddress;
					},
					[](const auto& amount) {
						return amount;
					})
	{}

	ResolverContext::ResolverContext(const MosaicResolver& mosaicResolver, const AddressResolver& addressResolver, const AmountResolver& amountResolver)
			: m_mosaicResolver(mosaicResolver)
			, m_addressResolver(addressResolver)
			, m_amountResolver(amountResolver)
	{}

	MosaicId ResolverContext::resolve(UnresolvedMosaicId mosaicId) const {
		return m_mosaicResolver(mosaicId);
	}

	Address ResolverContext::resolve(const UnresolvedAddress& address) const {
		return m_addressResolver(address);
	}

	Amount ResolverContext::resolve(const UnresolvedAmount& amount) const {
		return m_amountResolver(amount);
	}
}}
