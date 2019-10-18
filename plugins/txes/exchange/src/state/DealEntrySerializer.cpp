/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DealEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {

	void DealEntrySerializer::Save(const DealEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		io::Write(output, entry.transactionHash());
		io::Write(output, entry.deposit());

		io::Write8(output, static_cast<uint8_t>(entry.deposits().size()));
		for (const auto& pair : entry.deposits()) {
			io::Write(output, pair.first);
			io::Write(output, pair.second);
		}

		WriteOffers(entry.suggestedDeals(), output);

		io::Write8(output, static_cast<uint8_t>(entry.acceptedDeals().size()));
		for (const auto& pair : entry.acceptedDeals()) {
			io::Write(output, pair.first);
			WriteOffers(pair.second, output);
		}
	}

	DealEntry DealEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of DealEntry", version);

		auto transactionHash = io::Read<utils::ShortHash>(input);
		state::DealEntry entry(transactionHash);

		auto deposit = io::Read<Amount>(input);
		entry.setDeposit(deposit);

		auto depositCount = io::Read8(input);
		for (uint8_t i = 0; i < depositCount; ++i) {
			auto acceptedOfferTransactionHash = io::Read<utils::ShortHash>(input);
			auto acceptedOfferDeposit = io::Read<Amount>(input);
			entry.deposits().emplace(acceptedOfferTransactionHash, acceptedOfferDeposit);
		}

		ReadOffers(entry.suggestedDeals(), input);

		auto acceptedDealCount = io::Read8(input);
		for (uint8_t i = 0; i < acceptedDealCount; ++i) {
			transactionHash = io::Read<utils::ShortHash>(input);
			auto& acceptedOffers = entry.acceptedDeals()[transactionHash];
			ReadOffers(acceptedOffers, input);
		}

		return entry;
	}
}}
