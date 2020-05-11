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

#pragma once
#include "catapult/types.h"
#include <functional>

namespace catapult { namespace model {

	/// Context used to resolve unresolved types.
	class ResolverContext {
	private:
		template<typename TUnresolved, typename TResolved>
		using Resolver = std::function<TResolved (const TUnresolved&)>;
		using MosaicResolver = Resolver<UnresolvedMosaicId, MosaicId>;
		using AddressResolver = Resolver<UnresolvedAddress, Address>;
		using AmountResolver = Resolver<UnresolvedAmount, Amount>;
		using LevyMosaicResolver = Resolver<UnresolvedLevyMosaicId, MosaicId>;
		using LevyAddressResolver = Resolver<UnresolvedLevyAddress, Address>;
		
	public:
		/// Creates a default context.
		ResolverContext();
		
		/// Creates a context around \a mosaicResolver, \a addressResolver, \a amountResolver and levy resolvers
		ResolverContext(const MosaicResolver &mosaicResolver, const AddressResolver &addressResolver,
						const AmountResolver &amountResolver);
		
		/// Creates a context around \a mosaicResolver, \a addressResolver and \a amountResolver.
		ResolverContext(const MosaicResolver& mosaicResolver, const AddressResolver& addressResolver,
						const AmountResolver& amountResolver, const LevyMosaicResolver& levyMosaicResolver,
						const LevyAddressResolver& levyAddressResolver);

/// Resolves mosaic id (\a mosaicId).
		MosaicId resolve(UnresolvedMosaicId mosaicId) const;

		/// Resolves \a address.
		Address resolve(const UnresolvedAddress& address) const;

		/// Resolves \a amount.
		Amount resolve(const UnresolvedAmount& amount) const;
		
		/// Resolves levy mosaic id (\a mosaicId).
		MosaicId resolve(UnresolvedLevyMosaicId mosaicId) const;
		
		/// Resolves \a levy address.
		Address resolve(const UnresolvedLevyAddress& address) const;
		
	private:
		MosaicResolver m_mosaicResolver;
		AddressResolver m_addressResolver;
		AmountResolver m_amountResolver;
		LevyMosaicResolver m_levyMosaicResolver;
		LevyAddressResolver m_levyAddressResolver;
	};
}}
