/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LiquidityProviderEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	namespace {

		void SaveHistoryObservation(io::OutputStream& output, const HistoryObservation& observation) {
			io::Write64(output, observation.m_rate.m_currencyAmount.unwrap());
			io::Write64(output, observation.m_rate.m_mosaicAmount.unwrap());
			io::Write64(output, observation.m_turnover.unwrap());
		}

		void SaveHistoryObservations(io::OutputStream& output, const std::deque<HistoryObservation>& observations) {
			io::Write16(output, observations.size());
			for (const auto& observation: observations) {
				SaveHistoryObservation(output, observation);
			}
		}

		void LoadHistoryObservation(io::InputStream& input, HistoryObservation& observation) {
			ExchangeRate rate;
			rate.m_currencyAmount = Amount(io::Read64(input));
			rate.m_mosaicAmount = Amount(io::Read64(input));
			observation.m_rate = rate;
			observation.m_turnover = Amount(io::Read64(input));
		}

		void LoadHistoryObservations(io::InputStream& input, std::deque<HistoryObservation>& observations) {
			uint16_t size = io::Read16(input);
			for (int i = 0; i < size; i++) {
				HistoryObservation observation;
				LoadHistoryObservation(input, observation);
				observations.push_back(observation);
			}
		}
	}

	void LiquidityProviderEntrySerializer::Save(const LiquidityProviderEntry& entry, io::OutputStream& output) {

		io::Write32(output, entry.version());
		io::Write64(output, entry.mosaicId().unwrap());

		io::Write(output, entry.providerKey());
		io::Write(output, entry.owner());
		io::Write64(output, entry.additionallyMinted().unwrap());
		io::Write(output, entry.slashingAccount());
		io::Write32(output, entry.slashingPeriod());
		io::Write16(output, entry.windowSize());
		io::Write64(output, entry.creationHeight().unwrap());
		io::Write32(output, entry.alpha());
		io::Write32(output, entry.beta());

		SaveHistoryObservations(output, entry.turnoverHistory());
		SaveHistoryObservation(output, entry.recentTurnover());
	}

	LiquidityProviderEntry LiquidityProviderEntrySerializer::Load(io::InputStream& input) {

		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of DriveEntry", version);

		UnresolvedMosaicId mosaicId = UnresolvedMosaicId{io::Read64(input)};
		state::LiquidityProviderEntry entry(mosaicId);
		entry.setVersion(version);

		Key providerKey;
		io::Read(input, providerKey);
		entry.setProviderKey(providerKey);

		Key owner;
		io::Read(input, owner);
		entry.setOwner(owner);

		entry.setAdditionallyMinted(Amount{io::Read64(input)});

		Key slashingAccount;
		io::Read(input, slashingAccount);
		entry.setSlashingAccount(slashingAccount);

		entry.setSlashingPeriod(io::Read32(input));
		entry.setWindowSize(io::Read16(input));
		entry.setCreationHeight(Height(io::Read64(input)));
		entry.setAlpha(io::Read32(input));
		entry.setBeta(io::Read32(input));

		LoadHistoryObservations(input, entry.turnoverHistory());
		LoadHistoryObservation(input, entry.recentTurnover());

		return entry;
	}
}}
