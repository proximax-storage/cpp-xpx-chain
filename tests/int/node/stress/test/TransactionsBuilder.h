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

#include <plugins/txes/mosaic/src/model/MosaicLevy.h>
#include "BasicTransactionsBuilder.h"

namespace catapult { namespace test {

	/// Transactions builder and generator for transfer and namespace transactions.
	class TransactionsBuilder : public BasicTransactionsBuilder {
	private:
		// region descriptors

		struct NamespaceDescriptor {
			size_t OwnerId;
			std::string Name;
			BlockDuration Duration;
			size_t AddressAliasId; // optional
		};

		struct ModifyLevyDescriptor {
			size_t OwnerId;
			UnresolvedMosaicId MosaicId;
			model::MosaicLevyRaw Levy;
		};
		
		// endregion

	public:
		/// Creates a builder around \a accounts.
		explicit TransactionsBuilder(const Accounts& accounts);

	private:
		// BasicTransactionsBuilder
		model::UniqueEntityPtr<model::Transaction> generate(
				uint32_t descriptorType,
				const std::shared_ptr<const void>& pDescriptor,
				Timestamp deadline) const override;

	public:
		/// Adds a root namespace registration for namespace \a name by \a ownerId for specified \a duration,
		/// optionally setting an alias for \a aliasId.
		void addNamespace(size_t ownerId, const std::string& name, BlockDuration duration, size_t aliasId = 0);
		
		/// Adds a mosaic modify levy transaction with \a mosaic Id and \a levy data
		void addMosaicModifyLevy(size_t ownerId, const UnresolvedMosaicId& mosaicId, const model::MosaicLevyRaw& levy);
		
	private:
		model::UniqueEntityPtr<model::Transaction> createRegisterNamespace(const NamespaceDescriptor& descriptor, Timestamp deadline) const;

		model::UniqueEntityPtr<model::Transaction> createAddressAlias(const NamespaceDescriptor& descriptor, Timestamp deadline) const;
		
		model::UniqueEntityPtr<model::Transaction> createMosaicModifyLevy(const ModifyLevyDescriptor& descriptor, Timestamp deadline) const;

	private:
		enum class DescriptorType { Namespace = 1, Alias, MosaicModifyLevy };
	};
}}
