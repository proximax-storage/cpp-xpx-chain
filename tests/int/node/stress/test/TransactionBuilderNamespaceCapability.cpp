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

#include "TransactionBuilderNamespaceCapability.h"
#include "tests/test/local/RealTransactionFactory.h"

namespace catapult { namespace test {

	// region add / create


	void TransactionBuilderNamespaceCapability::registerHooks(){
		auto self = ptr<TransactionBuilderNamespaceCapability>();
		m_builder.registerDescriptor(DescriptorTypes::Namespace, [self](auto& pDescriptor, auto deadline){
		  return self->createRegisterNamespace(CastToDescriptor<NamespaceDescriptor>(pDescriptor), deadline);
		});
		m_builder.registerDescriptor(DescriptorTypes::Alias, [self](auto& pDescriptor, auto deadline){
		  return self->createAddressAlias(CastToDescriptor<NamespaceDescriptor>(pDescriptor), deadline);
		});
	}
	void TransactionBuilderNamespaceCapability::addNamespace(size_t ownerId, const std::string& name, BlockDuration duration, size_t aliasId) {
		auto descriptor = NamespaceDescriptor{ ownerId, name, duration, aliasId };
		add(DescriptorTypes::Namespace, descriptor);

		if (0 != descriptor.AddressAliasId)
			add(DescriptorTypes::Alias, descriptor);
	}


	model::UniqueEntityPtr<model::Transaction> TransactionBuilderNamespaceCapability::createRegisterNamespace(
			const NamespaceDescriptor& descriptor,
			Timestamp deadline) {
		const auto& ownerKeyPair = accounts().getKeyPair(descriptor.OwnerId);

		auto pTransaction = CreateRegisterRootNamespaceTransaction(ownerKeyPair, descriptor.Name, descriptor.Duration);
		return SignWithDeadline(std::move(pTransaction), ownerKeyPair, deadline);
	}



	model::UniqueEntityPtr<model::Transaction> TransactionBuilderNamespaceCapability::createAddressAlias(
			const NamespaceDescriptor& descriptor,
			Timestamp deadline) {
		const auto& ownerKeyPair = accounts().getKeyPair(descriptor.OwnerId);
		const auto& aliasedAddress = accounts().getAddress(descriptor.AddressAliasId);

		auto pTransaction = CreateRootAddressAliasTransaction(ownerKeyPair, descriptor.Name, aliasedAddress);
		return SignWithDeadline(std::move(pTransaction), ownerKeyPair, deadline);
	}

	// endregion
}}
