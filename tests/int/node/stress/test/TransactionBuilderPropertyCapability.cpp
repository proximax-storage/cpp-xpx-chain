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
