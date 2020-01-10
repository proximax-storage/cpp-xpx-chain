/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ConfigTreeCache.h"

namespace catapult { namespace config {

	bool ConfigTreeCache::contains(const Height& height) const {
		return m_configs.count(height) > 0;
	}

	bool ConfigTreeCache::containsRef(const Height& height) const {
		return m_references.count(height) > 0;
	}

	const BlockchainConfiguration& ConfigTreeCache::insert(const Height& height, const BlockchainConfiguration& config) {
		if (m_configs.count(height))
			CATAPULT_THROW_INVALID_ARGUMENT_1("duplicate config at height", height);

		auto pair = m_configs.emplace(height, ConfigRoot { config, {} });
		// Remove references to the previous config at heights where the new one applied.
		if (pair.first != m_configs.begin()) {
			auto references = (--pair.first)->second.Children;
			auto iter = references.lower_bound(height);
			while (iter != references.end()) {
				m_references.erase(*iter);
				iter = references.erase(iter);
			}
		}

		return m_configs.at(height).Config;
	}

	const BlockchainConfiguration& ConfigTreeCache::insertRef(const Height& refHeight, const Height& configHeight) {
		if (refHeight == configHeight)
			CATAPULT_THROW_INVALID_ARGUMENT_1("reference is not allowed at the same height", configHeight);

		auto iter = m_configs.find(configHeight);
		if (iter == m_configs.end())
			CATAPULT_THROW_INVALID_ARGUMENT_1(
					"failed to insert reference because config doesn't exist at height", configHeight);

		if (m_references.count(refHeight))
			CATAPULT_THROW_INVALID_ARGUMENT_1(
					"failed to insert reference because reference already exist at height", refHeight);

		auto& root = iter->second;
		cleanupRefs(root);
		m_references.emplace(refHeight, ConfigLeaf { root });
		root.Children.insert(refHeight);

		return iter->second.Config;
	}

	void ConfigTreeCache::erase(const Height& height) {
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

	const BlockchainConfiguration& ConfigTreeCache::get(const Height& height) {
		auto iterRef = m_references.find(height);
		if (iterRef != m_references.end())
			return iterRef->second.Parent.Config;

		auto iter = m_configs.find(height);
		if (iter != m_configs.end())
			return iter->second.Config;

		CATAPULT_THROW_INVALID_ARGUMENT_1("config doesn't exist at height", height);
	}
}} // namespace catapult::config