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

#include "Observers.h"
#include "plugins/txes/state_modify/src/utils/StateModifyApplicator.h"
#include "plugins/txes/mosaic/src/cache/MosaicCache.h"
#include "plugins/txes/multisig/src/cache/MultisigCache.h"
#include "plugins/txes/property/src/cache/PropertyCache.h"
#include "plugins/txes/upgrade/src/cache/BlockchainUpgradeCache.h"
#include "plugins/txes/metadata_v2/src/cache/MetadataCache.h"
#include "plugins/txes/config/src/cache/NetworkConfigCache.h"
#include "plugins/txes/metadata/src/cache/MetadataV1Cache.h"
#include "plugins/txes/exchange/src/cache/ExchangeCache.h"
#include "plugins/txes/storage/src/cache/DownloadChannelCache.h"
#include "plugins/txes/storage/src/cache/QueueCache.h"
#include "plugins/txes/storage/src/cache/PriorityQueueCache.h"
#include "plugins/txes/storage/src/cache/ReplicatorCache.h"
#include "plugins/txes/storage/src/cache/BcDriveCache.h"
#include "src/cache/LevyCache.h"

namespace catapult { namespace observers {

	namespace {
		void Observe(const model::ModifyStateNotification<1>& notification, const ObserverContext& context) {
			auto stateApplicator = utils::StateModifyApplicator(context.Cache);
			switch(notification.CacheId) {
			case cache::CacheId::NetworkConfig:
				stateApplicator.ApplyRecord<cache::NetworkConfigCache>(notification.KeyPtr, notification.ContentPtr);
				break;
			case cache::CacheId::AccountState:
				stateApplicator.ApplyRecord<cache::AccountStateCache>(notification.KeyPtr, notification.ContentPtr);
				break;
			case cache::CacheId::Namespace:
				switch(static_cast<int>(notification.SubCacheId)) {
				case static_cast<int>(cache::GeneralSubCache::Main):
					stateApplicator.ApplyRecord<cache::NamespaceCache>(notification.KeyPtr, notification.ContentPtr);
					break;
				case static_cast<int>(cache::GeneralSubCache::Secondary):
					stateApplicator.ApplyRecord<cache::NamespaceCache, cache::NamespaceFlatMapTypesSerializer>(notification.KeyPtr, notification.ContentPtr);
					break;
				case static_cast<int>(cache::GeneralSubCache::Height):
					stateApplicator.ApplyRecord<cache::NamespaceCache, cache::NamespaceHeightGroupingSerializer>(notification.KeyPtr, notification.ContentPtr);
					break;
				}
				break;
			case cache::CacheId::Metadata:
				stateApplicator.ApplyRecord<cache::MetadataV1Cache>(notification.KeyPtr, notification.ContentPtr);
				break;
			case cache::CacheId::Mosaic:
				stateApplicator.ApplyRecord<cache::MosaicCache>(notification.KeyPtr, notification.ContentPtr);
				break;
			case cache::CacheId::Multisig:
				stateApplicator.ApplyRecord<cache::MultisigCache>(notification.KeyPtr, notification.ContentPtr);
				break;
			case cache::CacheId::Property:
				stateApplicator.ApplyRecord<cache::PropertyCache>(notification.KeyPtr, notification.ContentPtr);
				break;
			case cache::CacheId::BlockchainUpgrade:
				stateApplicator.ApplyRecord<cache::BlockchainUpgradeCache>(notification.KeyPtr, notification.ContentPtr);
				break;
			case cache::CacheId::Exchange:
				stateApplicator.ApplyRecord<cache::ExchangeCache>(notification.KeyPtr, notification.ContentPtr);
				break;
			case cache::CacheId::MosaicLevy:
				stateApplicator.ApplyRecord<cache::LevyCache>(notification.KeyPtr, notification.ContentPtr);
				break;
			case cache::CacheId::Metadata_v2:
				stateApplicator.ApplyRecord<cache::MetadataCache>(notification.KeyPtr, notification.ContentPtr);
				break;
			case cache::CacheId::DownloadChannel:
				stateApplicator.ApplyRecord<cache::DownloadChannelCache>(notification.KeyPtr, notification.ContentPtr);
				break;
			case cache::CacheId::BcDrive:
				stateApplicator.ApplyRecord<cache::BcDriveCache>(notification.KeyPtr, notification.ContentPtr);
				break;
			case cache::CacheId::PriorityQueue:
				stateApplicator.ApplyRecord<cache::PriorityQueueCache>(notification.KeyPtr, notification.ContentPtr);
				break;
			case cache::CacheId::Replicator:
				stateApplicator.ApplyRecord<cache::ReplicatorCache>(notification.KeyPtr, notification.ContentPtr);
				break;
			case cache::CacheId::Queue:
				stateApplicator.ApplyRecord<cache::QueueCache>(notification.KeyPtr, notification.ContentPtr);
				break;
			default:
				break;
			}

		}
	}
	DEFINE_OBSERVER(ModifyState, model::ModifyStateNotification<1>, Observe);
}}
