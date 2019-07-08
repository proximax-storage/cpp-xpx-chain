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
#include "VerifiableEntity.h"
#include "catapult/model/Block.h"
#include "catapult/utils/HexFormatter.h"
#include <iosfwd>
#include <vector>

namespace catapult { namespace model {

	/// Wrapper around a strongly typed entity and its associated metadata.
	template<typename TEntity>
	class WeakEntityInfoT {
	public:
		/// Creates an entity info around \a associatedHeight.
		constexpr WeakEntityInfoT(const Height& associatedHeight = Height{0})
				: m_pEntity(nullptr)
				, m_pHash(nullptr)
				, m_pAssociatedBlockHeader(nullptr)
				, m_associatedHeight(associatedHeight)
		{}

		/// Creates an entity info around \a entity and \a associatedHeight.
		/// \note This is an implicit constructor in order to facilitate calling TransactionPlugin::publish without hashes.
		constexpr WeakEntityInfoT(const TEntity& entity, const Height& associatedHeight)
				: m_pEntity(&entity)
				, m_pHash(nullptr)
				, m_pAssociatedBlockHeader(nullptr)
				, m_associatedHeight(associatedHeight)
		{}

		/// Creates an entity info around \a entity and \a hash.
		constexpr explicit WeakEntityInfoT(const TEntity& entity, const Hash256& hash, const Height& associatedHeight)
				: m_pEntity(&entity)
				, m_pHash(&hash)
				, m_pAssociatedBlockHeader(nullptr)
				, m_associatedHeight(associatedHeight)
		{}

		/// Creates an entity info around \a entity, \a hash and \a associatedBlockHeader.
		constexpr explicit WeakEntityInfoT(const TEntity& entity, const Hash256& hash, const model::BlockHeader& associatedBlockHeader)
				: m_pEntity(&entity)
				, m_pHash(&hash)
				, m_pAssociatedBlockHeader(&associatedBlockHeader)
				, m_associatedHeight(associatedBlockHeader.Height)
		{}

	public:
		/// Returns \c true if this info has an associated entity.
		constexpr bool isSet() const {
			return !!m_pEntity;
		}

		/// Returns \c true if this info has an associated hash.
		constexpr bool isHashSet() const {
			return !!m_pHash;
		}

		/// Returns \c true if this info has an associated block header.
		constexpr bool isAssociatedBlockHeaderSet() const {
			return !!m_pAssociatedBlockHeader;
		}

	public:
		/// Gets the entity.
		constexpr const TEntity& entity() const {
			return *m_pEntity;
		}

		/// Gets the entity type.
		constexpr EntityType type() const {
			return m_pEntity->Type;
		}

		/// Gets the entity hash.
		constexpr const Hash256& hash() const {
			return *m_pHash;
		}

		/// Gets the associated block header.
		constexpr const model::BlockHeader& associatedBlockHeader() const {
			return *m_pAssociatedBlockHeader;
		}

		/// Gets the associated height.
		constexpr Height associatedHeight() const {
			return m_associatedHeight;
		}

	public:
		/// Coerces this info into a differently typed info.
		template<typename TEntityResult>
		WeakEntityInfoT<TEntityResult> cast() const {
			const auto& typedEntity = static_cast<const TEntityResult&>(entity());
			return isAssociatedBlockHeaderSet()
					? WeakEntityInfoT<TEntityResult>(typedEntity, hash(), associatedBlockHeader())
					: WeakEntityInfoT<TEntityResult>(typedEntity, hash(), m_associatedHeight);
		}

	public:
		/// Returns \c true if this info is equal to \a rhs.
		constexpr bool operator==(const WeakEntityInfoT& rhs) const {
			return m_pEntity == rhs.m_pEntity && m_pHash == rhs.m_pHash;
		}

		/// Returns \c true if this info is not equal to \a rhs.
		constexpr bool operator!=(const WeakEntityInfoT& rhs) const {
			return !(*this == rhs);
		}

	private:
		const TEntity* m_pEntity;
		const Hash256* m_pHash;
		const model::BlockHeader* m_pAssociatedBlockHeader;
		Height m_associatedHeight;
	};

	using WeakEntityInfo = WeakEntityInfoT<VerifiableEntity>;

	/// Insertion operator for outputting \a entityInfo to \a out.
	template<typename TEntity>
	std::ostream& operator<<(std::ostream& out, const WeakEntityInfoT<TEntity>& entityInfo) {
		if (!entityInfo.isSet()) {
			out << "WeakEntityInfo (unset)";
		} else {
			const auto& hash = entityInfo.hash();
			out << entityInfo.entity() << " [" << utils::HexFormat(hash.cbegin(), hash.cend()) << "]";
		}

		return out;
	}

	/// A container of weak entity infos.
	using WeakEntityInfos = std::vector<WeakEntityInfo>;
}}
