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
#include "ExternalCacheStorage.h"
#include "catapult/consumers/StateChangeInfo.h"
#include "catapult/subscribers/StateChangeSubscriber.h"

namespace catapult { namespace mongo {

	/// Api state change subscriber.
	class ApiStateChangeSubscriber : public subscribers::StateChangeSubscriber {
	public:
		/// Creates a subscriber around \a pCacheStorage.
		ApiStateChangeSubscriber(
				std::unique_ptr<ExternalCacheStorage>&& pCacheStorage)
				: m_pCacheStorage(std::move(pCacheStorage))
		{}

	public:
		void notifyStateChange(const consumers::StateChangeInfo& stateChangeInfo) override {
			m_pCacheStorage->saveDelta(stateChangeInfo.CacheDelta);
		}

	private:
		std::unique_ptr<ExternalCacheStorage> m_pCacheStorage;
	};
}}
