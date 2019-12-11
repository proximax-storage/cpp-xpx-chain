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
#include <cstring>

namespace catapult { namespace extensions {

	/// Casts \a mosaicId to an unresolved mosaic id.
	inline UnresolvedMosaicId CastToUnresolvedMosaicId(MosaicId mosaicId) {
		return UnresolvedMosaicId(mosaicId.unwrap());
	}

	/// Casts \a unresolvedMosaicId to a mosaic id.
	inline MosaicId CastToMosaicId(UnresolvedMosaicId unresolvedMosaicId) {
		return MosaicId(unresolvedMosaicId.unwrap());
	}

	namespace {
		template<typename TDest, typename TSource>
		TDest Copy(const TSource& source) {
			TDest dest;
			std::memcpy(dest.data(), source.data(), source.size());
			return dest;
		}
	}

	/// Copies \a address to an unresolved address.
	inline UnresolvedAddress CopyToUnresolvedAddress(const Address& address) {
		return Copy<UnresolvedAddress>(address);
	}

	/// Copies \a unresolvedAddress to an address.
	inline Address CopyToAddress(const UnresolvedAddress& unresolvedAddress) {
		return Copy<Address>(unresolvedAddress);
	}
}}
