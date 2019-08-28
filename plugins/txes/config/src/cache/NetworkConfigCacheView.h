/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "NetworkConfigBaseSets.h"
#include "NetworkConfigCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

	/// Mixins used by the network config cache view.
	using NetworkConfigCacheViewMixins = PatriciaTreeCacheMixins<NetworkConfigCacheTypes::PrimaryTypes::BaseSetType, NetworkConfigCacheDescriptor>;

	/// Basic view on top of the network config cache.
	class BasicNetworkConfigCacheView
			: public utils::MoveOnly
			, public NetworkConfigCacheViewMixins::Size
			, public NetworkConfigCacheViewMixins::Contains
			, public NetworkConfigCacheViewMixins::Iteration
			, public NetworkConfigCacheViewMixins::ConstAccessor
			, public NetworkConfigCacheViewMixins::PatriciaTreeView
			, public NetworkConfigCacheViewMixins::Enable
			, public NetworkConfigCacheViewMixins::Height {
	public:
		using ReadOnlyView = NetworkConfigCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a networkConfigSets.
		explicit BasicNetworkConfigCacheView(const NetworkConfigCacheTypes::BaseSets& networkConfigSets)
				: NetworkConfigCacheViewMixins::Size(networkConfigSets.Primary)
				, NetworkConfigCacheViewMixins::Contains(networkConfigSets.Primary)
				, NetworkConfigCacheViewMixins::Iteration(networkConfigSets.Primary)
				, NetworkConfigCacheViewMixins::ConstAccessor(networkConfigSets.Primary)
				, NetworkConfigCacheViewMixins::PatriciaTreeView(networkConfigSets.PatriciaTree.get())
				, m_networkConfigHeights(networkConfigSets.Heights)
		{}

	public:
		using NetworkConfigCacheViewMixins::ConstAccessor::find;

	public:
		Height FindConfigHeightAt(const Height& height) const {
			if (!height.unwrap())
				return height;

			auto iter = m_networkConfigHeights.findLowerOrEqual(height);
			if (!iter.get()) {
				return Height{0};
			}

			return *iter.get();
		}

	private:
		const NetworkConfigCacheTypes::HeightTypes::BaseSetType& m_networkConfigHeights;
	};

	/// View on top of the network config cache.
	class NetworkConfigCacheView : public ReadOnlyViewSupplier<BasicNetworkConfigCacheView> {
	public:
		/// Creates a view around \a networkConfigSets.
		explicit NetworkConfigCacheView(const NetworkConfigCacheTypes::BaseSets& networkConfigSets)
				: ReadOnlyViewSupplier(networkConfigSets)
		{}
	};
}}
