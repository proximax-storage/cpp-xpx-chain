/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SdaOfferGroupBaseSets.h"
#include "SdaOfferGroupCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	/// Mixins used by the SDA-SDA exchange cache view.
	using SdaOfferGroupCacheViewMixins = PatriciaTreeCacheMixins<SdaOfferGroupCacheTypes::PrimaryTypes::BaseSetType, SdaOfferGroupCacheDescriptor>;

	/// Basic view on top of the SDA-SDA exchange cache.
	class BasicSdaOfferGroupCacheView
			: public utils::MoveOnly
			, public SdaOfferGroupCacheViewMixins::Size
			, public SdaOfferGroupCacheViewMixins::Contains
			, public SdaOfferGroupCacheViewMixins::Iteration
			, public SdaOfferGroupCacheViewMixins::ConstAccessor
			, public SdaOfferGroupCacheViewMixins::PatriciaTreeView {
	public:
		using ReadOnlyView = SdaOfferGroupCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a sdaOfferGroupSets.
		explicit BasicSdaOfferGroupCacheView(
			const SdaOfferGroupCacheTypes::BaseSets& sdaOfferGroupSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: SdaOfferGroupCacheViewMixins::Size(sdaOfferGroupSets.Primary)
				, SdaOfferGroupCacheViewMixins::Contains(sdaOfferGroupSets.Primary)
				, SdaOfferGroupCacheViewMixins::Iteration(sdaOfferGroupSets.Primary)
				, SdaOfferGroupCacheViewMixins::ConstAccessor(sdaOfferGroupSets.Primary)
                , SdaOfferGroupCacheViewMixins::PatriciaTreeView(sdaOfferGroupSets.PatriciaTree.get())
		{}
	};

	/// View on top of the SDA-SDA exchange cache.
	class SdaOfferGroupCacheView : public ReadOnlyViewSupplier<BasicSdaOfferGroupCacheView> {
	public:
		/// Creates a view around \a sdaOfferGroupSets.
		explicit SdaOfferGroupCacheView(
			const SdaOfferGroupCacheTypes::BaseSets& sdaOfferGroupSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(sdaOfferGroupSets, pConfigHolder)
		{}
	};
}}
