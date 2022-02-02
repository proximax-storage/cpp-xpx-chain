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
#include "CompactMosaicMap.h"
#include "catapult/utils/Hashers.h"
#include "catapult/exceptions.h"
#include "catapult/model/BalanceSnapshot.h"
#include "catapult/model/NetworkConfiguration.h"
#include "catapult/types.h"
#include "catapult/utils/Hashers.h"
#include <boost/iterator/zip_iterator.hpp>
#include <list>

namespace catapult { namespace state {
	struct AccountState;

	/// Container holding information about account.
	class MosaicUnlockRequest {
	public:
		/// Creates an empty account balances.
		explicit MosaicUnlockRequest(Amount mosaicAmount, Height requestHeight) :
			m_Amount(mosaicAmount),
			m_Height(requestHeight)
		{
		}

	private:
		MosaicId m_MosaicId;
		Amount m_Amount;
		Height m_Height;

	};
}}
