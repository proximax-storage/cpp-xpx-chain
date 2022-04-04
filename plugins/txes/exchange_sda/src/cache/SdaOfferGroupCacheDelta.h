/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SdaOfferGroupBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include "src/config/SdaExchangeConfiguration.h"

namespace catapult { namespace cache {

    /// Mixins used by the SDA-SDA exchange cache delta.
    struct SdaOfferGroupCacheDeltaMixins {
    private:
        using PrimaryMixins = PatriciaTreeCacheMixins<SdaOfferGroupCacheTypes::PrimaryTypes::BaseSetDeltaType, SdaOfferGroupCacheDescriptor>;
    
    public:
        using Size = PrimaryMixins::Size;
        using Contains = PrimaryMixins::Contains;
        using ConstAccessor = PrimaryMixins::ConstAccessor;
        using MutableAccessor = PrimaryMixins::MutableAccessor;
        using PatriciaTreeDelta = PrimaryMixins::PatriciaTreeDelta;
        using BasicInsertRemove = PrimaryMixins::BasicInsertRemove;
        using DeltaElements = PrimaryMixins::DeltaElements;
        using ConfigBasedEnable = PrimaryMixins::ConfigBasedEnable<config::SdaExchangeConfiguration>;
    };

    /// Basic delta on top of the SDA-SDA exchange cache.
    class BasicSdaOfferGroupCacheDelta
            : public utils::MoveOnly
            , public SdaOfferGroupCacheDeltaMixins::Size
            , public SdaOfferGroupCacheDeltaMixins::Contains
            , public SdaOfferGroupCacheDeltaMixins::ConstAccessor
            , public SdaOfferGroupCacheDeltaMixins::MutableAccessor
            , public SdaOfferGroupCacheDeltaMixins::PatriciaTreeDelta
            , public SdaOfferGroupCacheDeltaMixins::BasicInsertRemove
            , public SdaOfferGroupCacheDeltaMixins::DeltaElements
            , public SdaOfferGroupCacheDeltaMixins::ConfigBasedEnable {
    public:
        using ReadOnlyView = SdaOfferGroupCacheTypes::CacheReadOnlyType;

    public:
        /// Creates a delta around \a sdaOfferGroupSets.
        explicit BasicSdaOfferGroupCacheDelta(
            const SdaOfferGroupCacheTypes::BaseSetDeltaPointers& sdaOfferGroupSets,
            std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
                    : SdaOfferGroupCacheDeltaMixins::Size(*sdaOfferGroupSets.pPrimary)
                    , SdaOfferGroupCacheDeltaMixins::Contains(*sdaOfferGroupSets.pPrimary)
                    , SdaOfferGroupCacheDeltaMixins::ConstAccessor(*sdaOfferGroupSets.pPrimary)
                    , SdaOfferGroupCacheDeltaMixins::MutableAccessor(*sdaOfferGroupSets.pPrimary)
                    , SdaOfferGroupCacheDeltaMixins::PatriciaTreeDelta(*sdaOfferGroupSets.pPrimary, sdaOfferGroupSets.pPatriciaTree)
                    , SdaOfferGroupCacheDeltaMixins::BasicInsertRemove(*sdaOfferGroupSets.pPrimary)
                    , SdaOfferGroupCacheDeltaMixins::DeltaElements(*sdaOfferGroupSets.pPrimary)
                    , SdaOfferGroupCacheDeltaMixins::ConfigBasedEnable(pConfigHolder, [](const auto& config) { return config.Enabled; })
                    , m_pSdaOfferGroupEntries(sdaOfferGroupSets.pPrimary)
        {}

    public:
        using SdaOfferGroupCacheDeltaMixins::ConstAccessor::find;
        using SdaOfferGroupCacheDeltaMixins::MutableAccessor::find;

    private:
        SdaOfferGroupCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pSdaOfferGroupEntries;
    };

    /// Delta on top of the SDA-SDA exchange cache.
    class SdaOfferGroupCacheDelta : public ReadOnlyViewSupplier<BasicSdaOfferGroupCacheDelta> {
    public:
        /// Creates a delta around \a sdaOfferGroupSets.
        explicit SdaOfferGroupCacheDelta(
            const SdaOfferGroupCacheTypes::BaseSetDeltaPointers& sdaOfferGroupSets,
            std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
                : ReadOnlyViewSupplier(sdaOfferGroupSets, pConfigHolder)
        {}
    };
}}
