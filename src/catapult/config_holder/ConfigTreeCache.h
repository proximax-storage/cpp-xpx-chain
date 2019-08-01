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
			uint64_t RootHeight;
			CatapultConfiguration Config;
			std::set<uint64_t, std::less<>> References;
		};

	public:
		explicit ConfigTreeCache() = default;

	public:
		bool contains(const Height& height) {
			return m_references.count(height.unwrap()) || m_configs.count(height.unwrap());
		}

		CatapultConfiguration& insert(const Height& height, const CatapultConfiguration& config) {
			if (m_configs.count(height.unwrap()))
				CATAPULT_THROW_INVALID_ARGUMENT_1("duplicate config at height ", height.unwrap());

			ConfigRoot root = { height.unwrap(), config , {} };
			m_configs.insert({ root.RootHeight, root });

			return m_configs.at(root.RootHeight).Config;
		}

		CatapultConfiguration& insertRef(const Height& heightOfRef, const Height& heightOfConfig) {
			auto iter = m_configs.find(heightOfConfig.unwrap());
			if (iter == m_configs.end())
				CATAPULT_THROW_INVALID_ARGUMENT_1("can't insert reference on config at height ", heightOfConfig);

			if (heightOfRef == heightOfConfig)
				CATAPULT_THROW_INVALID_ARGUMENT_1("can't insert reference on self at height ", heightOfConfig);

			removeOldReferences(iter->second);
			m_references.insert({ heightOfRef.unwrap(), iter->second });
			iter->second.References.insert(heightOfRef.unwrap());

			return iter->second.Config;
		}

		void erase(const Height& height) {
			auto iterRef = m_references.find(height.unwrap());
			if (iterRef != m_references.end()) {
				iterRef->second.References.erase(height.unwrap());
				m_references.erase(iterRef);
				return;
			}

			auto iter = m_configs.find(height.unwrap());
			if (iter != m_configs.end()) {
				reduceReferences(iter->second, 0);
				m_configs.erase(iter);
			}
		}

		CatapultConfiguration& get(const Height& height) {
			auto iterRef = m_references.find(height.unwrap());
			if (iterRef != m_references.end())
				return iterRef->second.Config;

			auto iter = m_configs.find(height.unwrap());
			if (iter != m_configs.end())
				return iter->second.Config;

			CATAPULT_THROW_INVALID_ARGUMENT_1("doesn't have config at height ", height);
		}

	private:
		void removeOldReferences(ConfigRoot& root) {
			reduceReferences(root, root.Config.BlockChain.MaxRollbackBlocks);
		}

		inline void reduceReferences(ConfigRoot& root, uint64_t size) {
			while (root.References.size() > size) {
				m_references.erase(*root.References.begin());
				root.References.erase(root.References.begin());
			}
		}

	private:
		/// Pair Height of config and config
		std::map<uint64_t, ConfigRoot> m_configs;

		/// Pair height of ref and reference on config
		std::map<uint64_t, ConfigRoot&> m_references;
	};
}}
