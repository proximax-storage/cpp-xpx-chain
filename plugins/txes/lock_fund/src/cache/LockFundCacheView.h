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
			using LookupMixin = LockFundLookupMixin<LockFundCacheTypes::PrimaryTypes::BaseSetType, LockFundCacheTypes::KeyedLockFundTypes::BaseSetType>;
			using Height = PrimaryMixins::Height;
			using Size = LockFundSizeMixin<LockFundCacheTypes::PrimaryTypes::BaseSetType, LockFundCacheTypes::KeyedLockFundTypes::BaseSetType>;
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
						, public LockFundCacheViewMixins::PrimaryMixins::Enable
						, public LockFundCacheViewMixins::Height{
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
