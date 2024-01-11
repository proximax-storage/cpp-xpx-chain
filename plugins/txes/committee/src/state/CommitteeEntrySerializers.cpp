/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommitteeEntrySerializers.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"
#include "catapult/functions.h"

namespace catapult { namespace state {

	namespace {
		using SaveCommitteeEntryCallback = consumer<const CommitteeEntry&, io::OutputStream&>;
		using LoadCommitteeEntryCallback = consumer<CommitteeEntry&, io::InputStream&>;

		void SaveCommitteeEntry(const CommitteeEntry& entry, io::OutputStream& output, const SaveCommitteeEntryCallback& callback) {
			// write version
			io::Write32(output, entry.version());

			io::Write(output, entry.key());
			io::Write(output, entry.owner());
			io::Write64(output, entry.disabledHeight().unwrap());
			io::Write64(output, entry.lastSigningBlockHeight().unwrap());
			io::Write64(output, entry.effectiveBalance().unwrap());
			io::Write8(output, entry.canHarvest());

			callback(entry, output);

			if (entry.version() > 2) {
				io::Write64Signed(output, entry.activity());
				io::Write32(output, entry.feeInterest());
				io::Write32(output, entry.feeInterestDenominator());
			}
		}

		CommitteeEntry LoadCommitteeEntry(io::InputStream& input, const LoadCommitteeEntryCallback& callback) {
			// read version
			VersionType version = io::Read32(input);
			if (version > 3)
				CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of CommitteeEntry", version);

			Key key;
			input.read(key);
			Key owner;
			input.read(owner);
			auto disabledHeight = Height{io::Read64(input)};
			auto lastSigningBlockHeight = Height{io::Read64(input)};
			auto effectiveBalance = Importance{io::Read64(input)};
			auto canHarvest = io::Read8(input);

			state::CommitteeEntry entry(key, owner, lastSigningBlockHeight, effectiveBalance, canHarvest, 0.0, 0.0, disabledHeight, version);

			callback(entry, input);

			if (version > 2) {
				entry.setActivity(io::Read64Signed(input));
				entry.setFeeInterest(io::Read32(input));
				entry.setFeeInterestDenominator(io::Read32(input));
			}

			return entry;
		}
	}

	void CommitteeEntrySerializer::Save(const CommitteeEntry& entry, io::OutputStream& output) {
		SaveCommitteeEntry(entry, output, [](const CommitteeEntry& entry, io::OutputStream& output) {
			io::WriteDouble(output, entry.activityObsolete());
			io::WriteDouble(output, entry.greedObsolete());

			if (entry.version() > 1)
				io::Write64(output, entry.expirationTime().unwrap());
		});
	}

	CommitteeEntry CommitteeEntrySerializer::Load(io::InputStream& input) {
		auto entry = LoadCommitteeEntry(input, [](CommitteeEntry& entry, io::InputStream& input) {
			entry.setActivityObsolete(io::ReadDouble(input));
			entry.setGreedObsolete(io::ReadDouble(input));

			if (entry.version() > 1)
				entry.setExpirationTime(Timestamp{io::Read64(input)});
		});

		return entry;
	}

	void CommitteeEntryPatriciaTreeSerializer::Save(const CommitteeEntry& entry, io::OutputStream& output) {
		SaveCommitteeEntry(entry, output, [](const CommitteeEntry&, io::OutputStream&){});
	}

	CommitteeEntry CommitteeEntryPatriciaTreeSerializer::Load(io::InputStream& input) {
		return LoadCommitteeEntry(input, [](CommitteeEntry&, io::InputStream&){});
	}
}}
