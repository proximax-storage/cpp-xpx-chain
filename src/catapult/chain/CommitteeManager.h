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
		explicit Committee(uint16_t round = 0)
			: Round(round)
		{}

	public:
		/// Public keys of committee members excluding the block proposer.
		model::PublicKeySet Cosigners;

		/// Public key of the block proposer.
		Key BlockProposer;

		/// Committee round (number of attempts to generate a block).
		uint16_t Round;
	};

	using BlockElementSupplier = supplier<std::shared_ptr<const model::BlockElement>>;

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
		const Committee& committee() {
			return m_committee;
		}

		/// Sets last block element \a supplier.
		void setLastBlockElementSupplier(const BlockElementSupplier& supplier);

		/// Gets last block element supplier.
		const BlockElementSupplier& lastBlockElementSupplier();

	protected:
		BlockElementSupplier m_lastBlockElementSupplier;
		Committee m_committee;
	};
}}
