/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/config/LockFundConfiguration.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "LockFundCacheTypes.h"
#include "LockFundBaseSets.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "LockFundCacheMixins.h"

namespace catapult { namespace cache {

		/// Mixins used by the lock fund cache view.
		struct LockFundCacheViewMixins {
		public:
			using PrimaryMixins = PatriciaTreeCacheMixins<LockFundCacheTypes::PrimaryTypes::BaseSetType, LockFundCacheDescriptor>;
			using KeyedMixins = BasicCacheMixins<LockFundCacheTypes::KeyedLockFundTypes::BaseSetType, LockFundCacheTypes::KeyedLockFundTypesDescriptor>;
			using LookupMixin = LockFundConstLookupMixin<LockFundCacheTypes::PrimaryTypes::BaseSetType, LockFundCacheTypes::KeyedLockFundTypes::BaseSetType>;
			using Size = LockFundSizeMixin<LockFundCacheTypes::PrimaryTypes::BaseSetType, LockFundCacheTypes::KeyedLockFundTypes::BaseSetType>;
			using ConfigBasedEnable = PrimaryMixins::ConfigBasedEnable<config::LockFundConfiguration>;
		};


		/// Basic view on top of the lock fund cache.
		class BasicLockFundCacheView
				: public utils::MoveOnly
						, public LockFundCacheViewMixins::Size
						, public LockFundCacheViewMixins::PrimaryMixins::Contains
						, public LockFundCacheViewMixins::PrimaryMixins::Iteration
						, public LockFundCacheViewMixins::LookupMixin
						, public LockFundCacheViewMixins::KeyedMixins::Contains
						, public LockFundCacheViewMixins::PrimaryMixins::PatriciaTreeView
						, public LockFundCacheViewMixins::PrimaryMixins::ActivePredicate
						, public LockFundCacheViewMixins::ConfigBasedEnable {
		public:
			using LockFundCacheViewMixins::KeyedMixins::Contains::contains;
			using LockFundCacheViewMixins::PrimaryMixins::Contains::contains;
			using ReadOnlyView = LockFundCacheTypes::CacheReadOnlyType;

		public:
			/// Creates a view around \a lockFundSets.
			explicit BasicLockFundCacheView(const LockFundCacheTypes::BaseSets& lockFundSets,
											std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
					: LockFundCacheViewMixins::Size(lockFundSets.Primary,  lockFundSets.KeyedInverseMap)
					, LockFundCacheViewMixins::PrimaryMixins::Contains(lockFundSets.Primary)
					, LockFundCacheViewMixins::PrimaryMixins::Iteration(lockFundSets.Primary)
					, LockFundCacheViewMixins::LookupMixin(lockFundSets.Primary, lockFundSets.KeyedInverseMap)
					, LockFundCacheViewMixins::KeyedMixins::Contains(lockFundSets.KeyedInverseMap)
					, LockFundCacheViewMixins::PrimaryMixins::PatriciaTreeView(lockFundSets.PatriciaTree.get())
					, LockFundCacheViewMixins::PrimaryMixins::ActivePredicate(lockFundSets.Primary)
					, LockFundCacheViewMixins::ConfigBasedEnable(pConfigHolder, [](const auto& config) { return config.Enabled; })
			{}
		};

		/// View on top of the lock fund cache.
		class LockFundCacheView : public ReadOnlyViewSupplier<BasicLockFundCacheView> {
		public:

			/// Creates a view around \a lockFundSets and \a args.
			template<typename... TArgs>
			explicit LockFundCacheView(const typename LockFundCacheTypes::BaseSets& lockFundSets,
									   std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder, TArgs&&... args)
					: ReadOnlyViewSupplier<BasicLockFundCacheView>(lockFundSets, pConfigHolder, std::forward<TArgs>(args)...)
			{}
		};
}}
