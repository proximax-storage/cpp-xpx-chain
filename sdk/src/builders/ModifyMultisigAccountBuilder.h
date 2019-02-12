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
#include "TransactionBuilder.h"
#include "plugins/txes/multisig/src/model/ModifyMultisigAccountTransaction.h"
#include <vector>

namespace catapult { namespace builders {

	/// Builder for a modify multisig account transaction.
	class ModifyMultisigAccountBuilder : public TransactionBuilder {
	public:
		using Transaction = model::ModifyMultisigAccountTransaction;
		using EmbeddedTransaction = model::EmbeddedModifyMultisigAccountTransaction;

	public:
		/// Creates a modify multisig account builder for building a modify multisig account transaction from \a signer
		/// for the network specified by \a networkIdentifier.
		ModifyMultisigAccountBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

	public:
		/// Sets the relative change of the minimal number of cosignatories required when removing an account to \a minRemovalDelta.
		void setMinRemovalDelta(int8_t minRemovalDelta);

		/// Sets the relative change of the minimal number of cosignatories required when approving a transaction to \a minApprovalDelta.
		void setMinApprovalDelta(int8_t minApprovalDelta);

		/// Adds \a modification to attached cosignatory modifications.
		void addModification(const model::CosignatoryModification& modification);

	public:
		/// Builds a new modify multisig account transaction.
		std::unique_ptr<Transaction> build() const;

		/// Builds a new embedded modify multisig account transaction.
		std::unique_ptr<EmbeddedTransaction> buildEmbedded() const;

	private:
		template<typename TTransaction>
		std::unique_ptr<TTransaction> buildImpl() const;

	private:
		int8_t m_minRemovalDelta;
		int8_t m_minApprovalDelta;
		std::vector<model::CosignatoryModification> m_modifications;
	};
}}
