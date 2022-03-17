/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "TransactionBuilderPropertyCapability.h"
#include "sdk/src/builders/AddressPropertyBuilder.h"
#include "sdk/src/extensions/ConversionExtensions.h"

namespace catapult { namespace test {

	namespace {
		constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;

	}
	// region add / create

	void TransactionBuilderPropertyCapability::addAddressBlockProperty(size_t senderId, size_t partnerId) {
		auto descriptor = PropertyAddressBlockDescriptor{ senderId, partnerId, true };
		add(DescriptorTypes::Property, descriptor);
	}

	void TransactionBuilderPropertyCapability::registerHooks() {
		m_builder.registerDescriptor(DescriptorTypes::Property, [self = ptr<TransactionBuilderPropertyCapability>()](auto& pDescriptor, auto deadline){
		  return self->createAddressPropertyTransaction(CastToDescriptor<PropertyAddressBlockDescriptor>(pDescriptor), deadline);
		});
	}
	void TransactionBuilderPropertyCapability::addAddressUnblockProperty(size_t senderId, size_t partnerId) {
		auto descriptor = PropertyAddressBlockDescriptor{ senderId, partnerId, false };
		add(DescriptorTypes::Property, descriptor);
	}

	model::UniqueEntityPtr<model::Transaction> TransactionBuilderPropertyCapability::createAddressPropertyTransaction(
			const PropertyAddressBlockDescriptor& descriptor,
			Timestamp deadline) {
		const auto& senderKeyPair = accounts().getKeyPair(descriptor.SenderId);
		auto partnerAddress = extensions::CopyToUnresolvedAddress(accounts().getAddress(descriptor.PartnerId));

		builders::AddressPropertyBuilder builder(Network_Identifier, senderKeyPair.publicKey());
		builder.setPropertyType(model::PropertyType::Block | model::PropertyType::Address);
		builder.addModification({
			descriptor.IsAdd ? model::PropertyModificationType::Add : model::PropertyModificationType::Del,
			partnerAddress
		});
		auto pTransaction = builder.build();

		return SignWithDeadline(std::move(pTransaction), senderKeyPair, deadline);
	}

	// endregion
}}
