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
#include <stdint.h>
#include "catapult/utils/Functional.h"

namespace catapult { namespace cache {

	using SubCacheId = uint8_t;
#define CACHE_ID_LIST \
	CATAPULT_ENUM_XLIST(NetworkConfig, \
				AccountState, \
				BlockDifficulty, \
				Hash, \
				Namespace, \
				Metadata, \
				Mosaic, \
				Multisig, \
				HashLockInfo, \
				SecretLockInfo, \
				Property, \
				Reputation, \
				Contract, \
				BlockchainUpgrade, \
				Drive, \
				Exchange, \
				Download, \
				SuperContract, \
				Operation, \
				MosaicLevy, \
				Metadata_v2,  \
				Committee, \
				BcDrive, \
				DownloadChannel, \
				Replicator, \
				Queue, \
				PriorityQueue, \
				Empty \
		)
#define CATAPULT_X_(data, elem, i) elem/**/BOOST_PP_COMMA_IF(BOOST_PP_NOT_EQUAL(i, BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(data))))
	CATAPULT_DECLARE_ENUM(CacheId, uint32_t, CACHE_ID_LIST)
#undef CATAPULT_X_
	CacheId GetCacheIdFromName(std::string name);

	std::string GetCacheNameFromCacheId(CacheId cacheId);
/// Defines cache constants for a cache with \a NAME.
#define DEFINE_CACHE_CONSTANTS(NAME) \
	static constexpr size_t Id = utils::to_underlying_type(CacheId::NAME); \
	static constexpr auto Name = #NAME "Cache";


	enum class GeneralSubCache : SubCacheId {
		Main,
		Height,
		Secondary
	};
}}
