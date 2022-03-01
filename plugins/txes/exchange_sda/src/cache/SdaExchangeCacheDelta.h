/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SdaExchangeBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include "src/config/SdaExchangeConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the SDA-SDA exchange cache delta.
	struct SdaExchangeCacheDeltaMixins : public PatriciaTreeCacheMixins<SdaExchangeCacheTypes::PrimaryTypes::BaseSetDeltaType, SdaExchangeCacheDescriptor> {
		using Pruning = HeightBasedPruningMixin<
			SdaExchangeCacheTypes::PrimaryTypes::BaseSetDeltaType,
			SdaExchangeCacheTypes::HeightGroupingTypes::BaseSetDeltaType>;
	};

	/// Basic delta on top of the SDA-SDA exchange cache.
	class BasicSdaExchangeCacheDelta
			: public utils::MoveOnly
			, public SdaExchangeCacheDeltaMixins::Size
			, public SdaExchangeCacheDeltaMixins::Contains
			, public SdaExchangeCacheDeltaMixins::ConstAccessor
			, public SdaExchangeCacheDeltaMixins::MutableAccessor
			, public SdaExchangeCacheDeltaMixins::PatriciaTreeDelta
			, public SdaExchangeCacheDeltaMixins::BasicInsertRemove
			, public SdaExchangeCacheDeltaMixins::Pruning
			, public SdaExchangeCacheDeltaMixins::DeltaElements
			, public SdaExchangeCacheDeltaMixins::ConfigBasedEnable<config::SdaExchangeConfiguration> {
	public:
		using ReadOnlyView = SdaExchangeCacheTypes::CacheReadOnlyType;
		using SdaOfferOwners = SdaExchangeCacheTypes::HeightGroupingTypes::BaseSetDeltaType::ElementType::Identifiers;

	public:
		/// Creates a delta around \a sdaExchangeSets.
		explicit BasicSdaExchangeCacheDelta(
			const SdaExchangeCacheTypes::BaseSetDeltaPointers& sdaExchangeSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder);

	public:
		using SdaExchangeCacheDeltaMixins::ConstAccessor::find;
		using SdaExchangeCacheDeltaMixins::MutableAccessor::find;

	public:
		/// Adds offer expiry \a height of \a owner.
		void addExpiryHeight(const SdaExchangeCacheDescriptor::KeyType& owner, const Height& height);

		/// Removes offer expiry \a height of \a owner.
		void removeExpiryHeight(const SdaExchangeCacheDescriptor::KeyType& owner, const Height& height);

		/// Returns owners of offers expiring at \a height.
		SdaOfferOwners expiringOfferOwners(Height height);

	private:
		SdaExchangeCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pSdaExchangeEntries;
		SdaExchangeCacheTypes::HeightGroupingTypes::BaseSetDeltaPointerType m_pHeightGroupingDelta;
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
	};

	/// Delta on top of the SDA-SDA exchange cache.
	class SdaExchangeCacheDelta : public ReadOnlyViewSupplier<BasicSdaExchangeCacheDelta> {
	public:
		/// Creates a delta around \a sdaExchangeSets.
		explicit SdaExchangeCacheDelta(
			const SdaExchangeCacheTypes::BaseSetDeltaPointers& sdaExchangeSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(sdaExchangeSets, pConfigHolder)
		{}
	};
}}
