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

#include "src/state/LockFundRecordSerializer.h"
#include "LockFundCacheTypes.h"
#include "catapult/cache/CacheStorageInclude.h"
namespace catapult { namespace cache {

		/// Policy for saving and loading lock fund cache data.
		struct LockFundCacheStorage
				: public CacheStorageForBasicInsertRemoveCache<LockFundCacheDescriptor>
						, public state::LockFundRecordSerializer<LockFundHeightIndexDescriptor> {

			/// Loads \a recordGroup into \a cacheDelta.
			static void LoadInto(const ValueType& history, DestinationType& cacheDelta);
		};
}}
