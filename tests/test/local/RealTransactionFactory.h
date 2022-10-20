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
#include "catapult/types.h"
#include "catapult/model/EntityPtr.h"
#include <memory>
#include <vector>

namespace catapult {
	namespace crypto { class KeyPair; }
	namespace model { struct Transaction; }
}

namespace catapult { namespace test {

	// notice that these helper functions create concrete transaction types
	// they are in local test utils because non-local tests should be using MockTransaction

	/// Creates basic unsigned transfer transaction with specified \a signerPublicKey, \a recipient and \a amount.
	model::UniqueEntityPtr<model::Transaction> CreateUnsignedTransferTransaction(
			const Key& signerPublicKey,
			const UnresolvedAddress& recipient,
			Amount amount);

	/// Creates basic signed transfer transaction with \a signer, \a recipient and \a amount.
	model::UniqueEntityPtr<model::Transaction> CreateTransferTransaction(
			const crypto::KeyPair& signer,
			const UnresolvedAddress& recipient,
			Amount amount);

	/// Creates basic signed transfer transaction with \a signer, \a recipientPublicKey and \a amount.
	model::UniqueEntityPtr<model::Transaction> CreateTransferTransaction(
			const crypto::KeyPair& signer,
			const Key& recipientPublicKey,
			Amount amount);

	/// Creates basic signed blockchain upgrade transaction with \a signer, \a config, \a supportedVersions and \a blocksBeforeActive.
	model::UniqueEntityPtr<model::Transaction> CreateNetworkConfigTransaction(
			const crypto::KeyPair& signer,
			const std::string& config,
			const std::string& supportedVersions,
			const BlockDuration& blocksBeforeActive);

	/// Creates basic signed blockchain upgrade transaction with \a signer, \a config, \a supportedVersions and \a activationHeight.
	model::UniqueEntityPtr<model::Transaction> CreateNetworkConfigAbsoluteHeightTransaction(
			const crypto::KeyPair& signer,
			const std::string& config,
			const std::string& supportedVersions,
			const Height& activationHeight);

	/// Creates signed root register namespace transaction with \a signer, \a name and \a duration.
	model::UniqueEntityPtr<model::Transaction> CreateRegisterRootNamespaceTransaction(
			const crypto::KeyPair& signer,
			const std::string& name,
			BlockDuration duration);

	/// Creates a signed root address alias transaction with \a signer, root namespace \a name and \a address.
	model::UniqueEntityPtr<model::Transaction> CreateRootAddressAliasTransaction(
			const crypto::KeyPair& signer,
			const std::string& name,
			const Address& address);
}}
