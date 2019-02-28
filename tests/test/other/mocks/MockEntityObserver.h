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
#include "catapult/model/Block.h"
#include "catapult/observers/ObserverTypes.h"
#include <vector>

namespace catapult { namespace mocks {

	/// Mock entity observer that captures information about observed entities and contexts.
	class MockEntityObserver : public observers::EntityObserver {
	public:
		/// Creates a mock observer with a default name,
		MockEntityObserver() : MockEntityObserver("MockObserverT")
		{}

		/// Creates a mock observer with \a name.
		explicit MockEntityObserver(const std::string& name) : m_name(name)
		{}

	public:
		const std::string& name() const override {
			return m_name;
		}

		void notify(const model::WeakEntityInfo& entityInfo, observers::ObserverContext& context) const override {
			m_entityHashes.push_back(entityInfo.hash());
			m_contexts.push_back(context);

			const auto& entity = entityInfo.entity();
			m_versions.push_back(entity.Version);
			if (model::BasicEntityType::Block == model::ToBasicEntityType(entity.Type))
				m_blockHeights.push_back(static_cast<const model::Block&>(entity).Height);
		}

	public:
		/// Returns collected block heights.
		const auto& blockHeights() const {
			return m_blockHeights;
		}

		/// Returns collected versions.
		const auto& versions() const {
			return m_versions;
		}

		/// Returns collected entity hashes.
		const auto& entityHashes() const {
			return m_entityHashes;
		}

		/// Returns collected contexts.
		const auto& contexts() const {
			return m_contexts;
		}

	private:
		std::string m_name;
		mutable std::vector<Height> m_blockHeights;
		mutable std::vector<uint16_t> m_versions;
		mutable std::vector<Hash256> m_entityHashes;
		mutable std::vector<observers::ObserverContext> m_contexts;
	};
}}
