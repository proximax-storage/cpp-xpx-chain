/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
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
