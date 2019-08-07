/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/config/CatapultConfiguration.h"
#include <set>

namespace catapult { namespace config {

	class ConfigTreeCache {
	private:
		struct ConfigRoot {
			CatapultConfiguration Config;
			std::set<Height> Children;
		};

		struct ConfigLeaf {
			ConfigRoot& Parent;
		};

	public:
		explicit ConfigTreeCache() = default;

	public:
		bool contains(const Height& height) const {
			return m_references.count(height) || m_configs.count(height);
		}

		CatapultConfiguration& insert(const Height& height, const CatapultConfiguration& config) {
			if (m_configs.count(height))
				CATAPULT_THROW_INVALID_ARGUMENT_1("duplicate config at height", height);

			m_configs.emplace(height, ConfigRoot{ config , {} });

			return m_configs.at(height).Config;
		}

		CatapultConfiguration& insertRef(const Height& refHeight, const Height& configHeight) {
			if (refHeight == configHeight)
				CATAPULT_THROW_INVALID_ARGUMENT_1("reference is not allowed at the same height", configHeight);

			auto iter = m_configs.find(configHeight);
			if (iter == m_configs.end())
				CATAPULT_THROW_INVALID_ARGUMENT_1("failed to insert reference because config doesn't exist at height", configHeight);

			auto& root = iter->second;
			cleanupRefs(root);
			m_references.emplace(refHeight, ConfigLeaf{ root });
			root.Children.insert(refHeight);

			return iter->second.Config;
		}

		void erase(const Height& height) {
			auto iterRef = m_references.find(height);
			if (iterRef != m_references.end()) {
				iterRef->second.Parent.Children.erase(height);
				m_references.erase(iterRef);
				return;
			}

			auto iter = m_configs.find(height);
			if (iter != m_configs.end()) {
				cleanupRefs(iter->second, 0);
				m_configs.erase(iter);
			}
		}

		CatapultConfiguration& get(const Height& height) {
			auto iterRef = m_references.find(height);
			if (iterRef != m_references.end())
				return iterRef->second.Parent.Config;

			auto iter = m_configs.find(height);
			if (iter != m_configs.end())
				return iter->second.Config;

			CATAPULT_THROW_INVALID_ARGUMENT_1("config doesn't exist at height", height);
		}

	private:
		void cleanupRefs(ConfigRoot& root) {
			cleanupRefs(root, root.Config.BlockChain.MaxRollbackBlocks);
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
