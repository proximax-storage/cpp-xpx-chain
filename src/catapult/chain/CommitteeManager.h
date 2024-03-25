/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Elements.h"
#include "catapult/utils/NonCopyable.h"

namespace catapult { namespace chain {

	constexpr auto CommitteePhaseCount = 4u;

	void IncreasePhaseTime(uint64_t& phaseTimeMillis, const model::NetworkConfiguration& config);
	void DecreasePhaseTime(uint64_t& phaseTimeMillis, const model::NetworkConfiguration& config);

	struct HarvesterWeight {
		union {
			double d;
			int64_t n;
		};
	};

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

		/// Selects new committee and increments round.
		virtual Key getBootKey(const Key& harvestKey, const model::NetworkConfiguration& config) const = 0;

		/// Resets committee manager to start a new block generation cycle.
		virtual void reset() = 0;

		/// Returns the committee selected by the last call of selectCommittee()
		/// or empty committee after reset().
		virtual Committee committee() const = 0;

		/// Sets last block element \a supplier.
		void setLastBlockElementSupplier(const model::BlockElementSupplier& supplier);

		/// Gets last block element supplier.
		const model::BlockElementSupplier& lastBlockElementSupplier();

		/// Returns the weight of the account by \a accountKey using \a config.
		virtual HarvesterWeight weight(const Key& accountKey, const model::NetworkConfiguration& config) const = 0;

		/// Adds \a delta to \a weight.
		virtual HarvesterWeight zeroWeight() const = 0;

		/// Adds \a delta to \a weight.
		virtual void add(HarvesterWeight& weight, const chain::HarvesterWeight& delta) const = 0;

		/// Multiplies \a weight by \a multiplier.
		virtual void mul(HarvesterWeight& weight, double multiplier) const = 0;

		/// Returns true if \a weight1 is greater or equals \a weight2, otherwise false.
		virtual bool ge(const HarvesterWeight& weight1, const chain::HarvesterWeight& weight2) const = 0;

		/// Returns true if \a weight1 equals \a weight2, otherwise false.
		virtual bool eq(const HarvesterWeight& weight1, const chain::HarvesterWeight& weight2) const = 0;

		/// Returns printable presentation of \a weight2.
		virtual std::string str(const HarvesterWeight& weight) const = 0;

		virtual void logCommittee() const = 0;

	protected:
		model::BlockElementSupplier m_lastBlockElementSupplier;
		Committee m_committee;
	};
}}
