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
#include "ContainerTypes.h"
#include "ExtractorContext.h"
#include "ResolverContext.h"
#include <functional>
#include "src/catapult/cache/CatapultCacheView.h"
#include "src/catapult/cache/ReadOnlyCatapultCache.h"

namespace catapult {
	namespace model {
		class NotificationPublisher;
		struct Transaction;
	}
	
	namespace util {
		
		using ReadOnlyCacheFactory = std::function<cache::ReadOnlyCatapultCache()>;
		using ResolverContextFactory = std::function<model::ResolverContext(cache::ReadOnlyCatapultCache&)>;
		
		struct ResolverContextHandle {
			ReadOnlyCacheFactory CacheFactory;
			ResolverContextFactory ResolverFactory;
			ResolverContextHandle(const ReadOnlyCacheFactory& cacheFactory, const ResolverContextFactory& resolverFactory)
				: CacheFactory(cacheFactory)
				, ResolverFactory(resolverFactory){
			}
		};
	}
}

namespace catapult { namespace model {
	
	/// Extracts all addresses that are involved in \a transaction at \a height with \a hash using \a notificationPublisher and \a extractorContext.
	UnresolvedAddressSet ExtractAddresses(const Transaction& transaction, const Hash256& hash, const Height& height,
	                                      const NotificationPublisher& notificationPublisher, const ExtractorContext& extractorContext);
		
	/// Extracts all addresses that are involved in \a transaction at \a height with \a hash using \a notificationPublisher and \a extractorContext and \a resolverContext
	UnresolvedAddressSet ExtractAddresses(const Transaction& transaction, const Hash256& hash, const Height& height,
			const NotificationPublisher& notificationPublisher, const ExtractorContext& extractorContext, const util::ResolverContextHandle& resolverHandle);
}}
