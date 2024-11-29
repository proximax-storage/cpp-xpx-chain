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

#include "catapult/model/Notifications.h"
#include "boost/core/span.hpp"
#include "catapult/cache/CacheConstants.h"

namespace catapult { namespace model {

	// region namespace notification types

	/// Defines a data modification notification type.
	DEFINE_NOTIFICATION_TYPE(All, ModifyState, Modify_State, 0x0001);

	/// Notification of a data modification.
	template<VersionType version>
	struct ModifyStateNotification;

	template<>
	struct ModifyStateNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ModifyState_Modify_State_Notification;

	public:
		explicit ModifyStateNotification(
				const cache::CacheId cacheId,
				const cache::SubCacheId subCacheId,
				const Key& signer,
				const uint8_t* keyPtr,
				const size_t keySize,
				const uint8_t* contentPtr,
				const size_t contentSize)
			: Notification(Notification_Type, sizeof(ModifyStateNotification<1>))
			, Signer(signer)
			, CacheId(cacheId)
			, SubCacheId(subCacheId)
			, KeyPtr(keyPtr, keySize)
			, ContentPtr(contentPtr, contentSize)
		{}

	public:
		/// FolderName length.
		cache::CacheId CacheId;
		cache::SubCacheId SubCacheId;
		boost::span<const uint8_t> KeyPtr;
		boost::span<const uint8_t> ContentPtr;
		const Key& Signer;
	};
}}
