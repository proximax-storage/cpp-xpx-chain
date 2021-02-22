/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Elements.h"
#include "catapult/utils/NonCopyable.h"

namespace catapult { namespace chain {

	struct Committee {
	public:
		explicit Committee(int64_t round = -1)
			: Round(round)
		{}

	public:
		/// Public keys of committee members excluding the block proposer.
		model::PublicKeySet Cosigners;

		/// Public key of the block proposer.
		Key BlockProposer;

		/// Committee round (number of attempts to generate a block).
		int64_t Round;
	};

	/// Interface for committee manager.
	class CommitteeManager : public utils::NonCopyable {
	public:
		virtual ~CommitteeManager() = default;

	public:
		/// Selects new committee and increments round.
		virtual const Committee& selectCommittee(const model::NetworkConfiguration& config) = 0;

		/// Resets committee manager to start a new block generation cycle.
		virtual void reset() = 0;

		/// Returns the committee selected by the last call of selectCommittee()
		/// or empty committee after reset().
		const Committee& committee() const {
			return m_committee;
		}

		/// Sets last block element \a supplier.
		void setLastBlockElementSupplier(const model::BlockElementSupplier& supplier);

		/// Gets last block element supplier.
		const model::BlockElementSupplier& lastBlockElementSupplier();

		/// Returns the weight of the account by \a accountKey.
		virtual double weight(const Key& accountKey) const = 0;

	protected:
		model::BlockElementSupplier m_lastBlockElementSupplier;
		Committee m_committee;
	};
}}
