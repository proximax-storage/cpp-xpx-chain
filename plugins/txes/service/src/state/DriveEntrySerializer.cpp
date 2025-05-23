/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DriveEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	namespace {

		void SavePayments(io::OutputStream &output, const std::vector<PaymentInformation>& payments) {
			io::Write16(output, payments.size());
			for (const auto& payment : payments) {
				io::Write(output, payment.Receiver);
				io::Write(output, payment.Amount);
				io::Write(output, payment.Height);
			}
		}

		void LoadPayments(io::InputStream& input, std::vector<PaymentInformation>& payments) {
			auto numPayments = io::Read16(input);
			while (numPayments--) {
				PaymentInformation payment;
				io::Read(input, payment.Receiver);
				io::Read(input, payment.Amount);
				io::Read(input, payment.Height);

				payments.emplace_back(payment);
			}
		}

		void SaveBillingHistory(io::OutputStream& output, const std::vector<BillingPeriodDescription>& billingHistory) {
			io::Write16(output, billingHistory.size());
			for (const auto& description : billingHistory) {
				io::Write(output, description.Start);
				io::Write(output, description.End);
				SavePayments(output, description.Payments);
			}
		}

		void LoadBillingHistory(io::InputStream& input, std::vector<BillingPeriodDescription>& billingHistory) {
			auto numDescriptions = io::Read16(input);
			while (numDescriptions--) {
				BillingPeriodDescription description;
				io::Read(input, description.Start);
				io::Read(input, description.End);

				LoadPayments(input, description.Payments);

				billingHistory.emplace_back(description);
			}
		}

		void SaveFiles(io::OutputStream& output, const state::FilesMap& filesMap) {
			io::Write16(output, filesMap.size());
			for (const auto& filePair : filesMap) {
				io::Write(output, filePair.first);
				io::Write64(output, filePair.second.Size);
			}
		}

		void LoadFiles(io::InputStream& input, state::FilesMap& filesMap) {
			auto numFiles = io::Read16(input);
			while (numFiles--) {
				Hash256 hashFile;
				io::Read(input, hashFile);

				FileInfo info;
				info.Size = io::Read64(input);
				filesMap.emplace(hashFile, info);
			}
		}

		template<typename T>
		void SaveReplicators(io::OutputStream& output, const T& replicatorsMap) {
			io::Write16(output, replicatorsMap.size());
			for (const auto& replicatorPair : replicatorsMap) {
				io::Write(output, replicatorPair.first);
				io::Write(output, replicatorPair.second.Start);
				io::Write(output, replicatorPair.second.End);

				const auto& activeDeposits = replicatorPair.second.ActiveFilesWithoutDeposit;
				io::Write16(output, activeDeposits.size());
				for (const auto& fileHash : activeDeposits) {
					io::Write(output, fileHash);
				}

				const auto& inactiveDeposits = replicatorPair.second.InactiveFilesWithoutDeposit;
				io::Write16(output, inactiveDeposits.size());
				for (const auto& fileHashPair : inactiveDeposits) {
					io::Write(output, fileHashPair.first);
					io::Write16(output, fileHashPair.second.size());
					for (const auto& height : fileHashPair.second)
						io::Write(output, height);
				}
			}
		}

		template<typename T>
		inline void emplace(std::vector<T>& replicators, T&& p) {
			replicators.emplace_back(p);
		}

		template<typename T1, typename T2>
		inline void emplace(std::map<T1, T2>& replicators, std::pair<T1, T2>&& p) {
			replicators.emplace(p);
		}

		template<typename T>
		void LoadReplicators(io::InputStream& input, T& replicatorsMap) {
			auto numReplicators = io::Read16(input);
			while (numReplicators--) {
				Key replicator;
				io::Read(input, replicator);

				ReplicatorInfo info;
				io::Read(input, info.Start);
				io::Read(input, info.End);

				auto numActiveDeposits = io::Read16(input);
				while (numActiveDeposits--) {
					Hash256 fileHash;
					io::Read(input, fileHash);

					info.ActiveFilesWithoutDeposit.emplace(fileHash);
				}

				auto numInactiveDeposits = io::Read16(input);
				while (numInactiveDeposits--) {
					Hash256 fileHash;
					io::Read(input, fileHash);

					std::vector<Height> heights(io::Read16(input));
					for (auto& height : heights)
						io::Read(input, height);

					info.InactiveFilesWithoutDeposit.emplace(fileHash, heights);
				}

				emplace(replicatorsMap, { replicator, info });
			}
		}
	}

	void DriveEntrySerializer::Save(const DriveEntry& driveEntry, io::OutputStream& output) {
		auto version = driveEntry.version();

		if (driveEntry.coowners().size() > 0) {
			version = std::max(VersionType(2), version);
		}

		io::Write32(output, version);

		io::Write(output, driveEntry.key());
		io::Write8(output, utils::to_underlying_type(driveEntry.state()));
		io::Write(output, driveEntry.owner());
		io::Write(output, driveEntry.rootHash());
		io::Write(output, driveEntry.start());
		io::Write(output, driveEntry.end());
		io::Write(output, driveEntry.duration());
		io::Write(output, driveEntry.billingPeriod());
		io::Write(output, driveEntry.billingPrice());

		SaveBillingHistory(output, driveEntry.billingHistory());

		io::Write64(output, driveEntry.size());
		io::Write64(output, driveEntry.occupiedSpace());
		io::Write16(output, driveEntry.replicas());
		io::Write16(output, driveEntry.minReplicators());
		io::Write8(output, driveEntry.percentApprovers());

		SaveFiles(output, driveEntry.files());
		SaveReplicators(output, driveEntry.replicators());
		SaveReplicators(output, driveEntry.removedReplicators());
		SavePayments(output, driveEntry.uploadPayments());

		if (version > 1) {
			io::Write16(output, driveEntry.coowners().size());
			for (const auto& coowner : driveEntry.coowners()) {
				io::Write(output, coowner);
			}
		}
	}

	DriveEntry DriveEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 3)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of DriveEntry", version);

		Key key;
		input.read(key);
		state::DriveEntry entry(key);
		entry.setVersion(version);

		entry.setState(static_cast<DriveState>(io::Read8(input)));

		Key owner;
		input.read(owner);
		entry.setOwner(owner);

		Hash256 rootHash;
		input.read(rootHash);
		entry.setRootHash(rootHash);

		entry.setStart(Height(io::Read64(input)));
		entry.setEnd(Height(io::Read64(input)));
		entry.setDuration(BlockDuration(io::Read64(input)));
		entry.setBillingPeriod(BlockDuration(io::Read64(input)));
		entry.setBillingPrice(Amount(io::Read64(input)));

		LoadBillingHistory(input, entry.billingHistory());

		entry.setSize(io::Read64(input));
		entry.setOccupiedSpace(io::Read64(input));
		entry.setReplicas(io::Read16(input));
		entry.setMinReplicators(io::Read16(input));
		entry.setPercentApprovers(io::Read8(input));

		LoadFiles(input, entry.files());
		LoadReplicators(input, entry.replicators());
		LoadReplicators(input, entry.removedReplicators());
		LoadPayments(input, entry.uploadPayments());

		if (version > 1) {
			auto numCoowners = io::Read16(input);
			while (numCoowners--) {
				Key coowner;
				io::Read(input, coowner);

				entry.coowners().insert(coowner);
			}
		}

		return entry;
	}
}}
