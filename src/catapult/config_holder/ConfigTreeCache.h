/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/config/BlockchainConfiguration.h"
#include <set>
#include <mutex>

namespace catapult { namespace config {

	class ConfigTreeCache {
	private:
		struct ConfigRoot {
			BlockchainConfiguration Config;
			std::set<Height> Children;
		};

		struct ConfigLeaf {
			ConfigRoot& Parent;
		};

	public:
		explicit ConfigTreeCache() = default;

	public:
		bool contains(const Height& height) const;

		bool containsRef(const Height& height) const;

		const BlockchainConfiguration& insert(const Height& height, const BlockchainConfiguration& config);

		const BlockchainConfiguration& insertRef(const Height& refHeight, const Height& configHeight);

		void erase(const Height& height);

		const BlockchainConfiguration& get(const Height& height);

	private:
		void cleanupRefs(ConfigRoot& root) {
			cleanupRefs(root, 4 * root.Config.Network.MaxRollbackBlocks);
		}

		inline void cleanupRefs(ConfigRoot& root, uint64_t size) {
			while (root.Children.size() > size) {
				m_references.erase(*root.Children.begin());
				root.Children.erase(root.Children.begin());
			}
		}

	private:
		/// Pair Height of config and config
		std::map<Height, ConfigRoot> m_configs;

		/// Pair height of ref and reference on config
		std::map<Height, ConfigLeaf> m_references;
	};
}}
