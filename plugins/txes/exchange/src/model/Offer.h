/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/exceptions.h"
#include "catapult/model/Mosaic.h"
#include "catapult/utils/ShortHash.h"
#include <cmath>

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Offer type
	enum class OfferType : uint8_t {
		/// Buy offer.
		Buy,

		/// Sell offer.
		Sell
	};

	struct Offer {
	public:
		/// Mosaic for exchange.
		UnresolvedMosaic Mosaic;

		/// Sum of XPX suggested to be paid for mosaic.
		Amount Cost;

	private:
		using AmountValueType = typeof(Amount::ValueType);

	public:

		Offer& operator+=(const Offer& rhs) {
			if (rhs.Mosaic.MosaicId != Mosaic.MosaicId)
				CATAPULT_THROW_INVALID_ARGUMENT_1("invalid mosaic", rhs.Mosaic.MosaicId)

			Mosaic.Amount = Amount(Mosaic.Amount.unwrap() + rhs.Mosaic.Amount.unwrap());
			Cost = Amount(Cost.unwrap() + rhs.Cost.unwrap());

			return *this;
		}

		Offer& operator-=(const Offer& rhs) {
			if (rhs.Mosaic.MosaicId != Mosaic.MosaicId)
				CATAPULT_THROW_INVALID_ARGUMENT_1("invalid mosaic", rhs.Mosaic.MosaicId)

			if (rhs.Mosaic.Amount > Mosaic.Amount)
				CATAPULT_THROW_INVALID_ARGUMENT_2("subtracting value greater than mosaic amount", rhs.Mosaic.Amount, Mosaic.Amount)

			if (rhs.Cost > Cost)
				CATAPULT_THROW_INVALID_ARGUMENT_2("subtracting value greater than mosaic cost", rhs.Cost, Cost)

			Mosaic.Amount = Amount(Mosaic.Amount.unwrap() - rhs.Mosaic.Amount.unwrap());
			Cost = Amount(Cost.unwrap() - rhs.Cost.unwrap());

			return *this;
		}


		Offer operator+(const Offer& rhs) {
			Offer offer{Mosaic, Cost};
			return offer.operator+=(rhs);
		}

		Offer operator-(const Offer& rhs) {
			Offer offer{Mosaic, Cost};
			return offer.operator-=(rhs);
		}

		bool operator==(const Offer& rhs) const {
			return price() == rhs.price();
		}

		bool operator!=(const Offer& rhs) const {
			return !(*this == rhs);
		}

		bool operator>=(const Offer& rhs) const {
			return price() >= rhs.price();
		}

		bool operator>(const Offer& rhs) const {
			return price() > rhs.price();
		}

		bool operator<=(const Offer& rhs) const {
			return price() <= rhs.price();
		}

		bool operator<(const Offer& rhs) const {
			return price() < rhs.price();
		}

		double price() const {
			return static_cast<double>(Mosaic.Amount.unwrap()) / static_cast<double>(Cost.unwrap());
		}

		Amount cost(const Amount& amount) const {
			auto cost = std::ceil(static_cast<double>(Mosaic.Amount.unwrap()) * static_cast<double>(amount.unwrap()) / static_cast<double>(Cost.unwrap()));
			return Amount(static_cast<AmountValueType>(cost));
		}
	};

	struct MatchedOffer : public Offer {
	public:
		/// The signer of the transaction with matched offer.
		Key TransactionSigner;

		/// The hash of the transaction with matched offer.
		utils::ShortHash TransactionHash;
	};

#pragma pack(pop)
}}
