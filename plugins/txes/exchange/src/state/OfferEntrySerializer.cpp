/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "OfferEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {

	void OfferEntrySerializer::Save(const OfferEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		io::Write(output, entry.transactionHash());
		io::Write(output, entry.transactionSigner());
		io::Write(output, entry.deadline());
		io::Write(output, entry.deposit());

		WriteOffers(entry.offers(), output);
	}

	OfferEntry OfferEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of OfferEntry", version);

		auto transactionHash = io::Read<utils::ShortHash>(input);
		Key transactionSigner;
		io::Read(input, transactionSigner);
		state::OfferEntry entry(transactionHash, transactionSigner);

		auto deadline = io::Read<Timestamp>(input);
		entry.setDeadline(deadline);

		auto deposit = io::Read<Amount>(input);
		entry.setDeposit(deposit);

		ReadOffers(entry.offers(), input);

		return entry;
	}
}}
