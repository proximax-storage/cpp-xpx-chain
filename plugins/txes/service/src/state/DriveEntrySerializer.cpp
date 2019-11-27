/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DriveEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"
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
				io::Write(output, filePair.second.Deposit);

				const auto& actions = filePair.second.Actions;
				io::Write16(output, actions.size());
				for (const auto& action : actions) {
					io::Write8(output, utils::to_underlying_type(action.Type));
					io::Write(output, action.ActionHeight);
				}

				SavePayments(output, filePair.second.Payments);
			}
		}

		void LoadFiles(io::InputStream& input, state::FilesMap& filesMap) {
			auto numFiles = io::Read16(input);
			while (numFiles--) {
				Hash256 hashFile;
				io::Read(input, hashFile);

				FileInfo info;
				info.Size = io::Read64(input);
				io::Read(input, info.Deposit);

				auto numActions = io::Read16(input);
				while (numActions--) {
					DriveAction action;
					action.Type = static_cast<DriveActionType>(io::Read8(input));
					io::Read(input, action.ActionHeight);

					info.Actions.emplace_back(action);
				}

				LoadPayments(input, info.Payments);

				filesMap.emplace(hashFile, info);
			}
		}

		void SaveReplicators(io::OutputStream& output, const state::ReplicatorsMap& replicatorsMap) {
			io::Write16(output, replicatorsMap.size());
			for (const auto& replicatorPair : replicatorsMap) {
				io::Write(output, replicatorPair.first);
				io::Write(output, replicatorPair.second.Start);
				io::Write(output, replicatorPair.second.End);
				io::Write(output, replicatorPair.second.Deposit);

				const auto& deposits = replicatorPair.second.FilesWithoutDeposit;
				io::Write16(output, deposits.size());
				for (const auto& pair : deposits) {
					io::Write(output, pair.first);
					io::Write16(output, pair.second);
				}
			}
		}

		void LoadReplicators(io::InputStream& input, state::ReplicatorsMap& replicatorsMap) {
			auto numReplicators = io::Read16(input);
			while (numReplicators--) {
				Key replicator;
				io::Read(input, replicator);

				ReplicatorInfo info;
				io::Read(input, info.Start);
				io::Read(input, info.End);
				io::Read(input, info.Deposit);

				auto numDeposits = io::Read16(input);
				while (numDeposits--) {
					Hash256 fileHash;
					io::Read(input, fileHash);
					auto count = io::Read16(input);

					info.FilesWithoutDeposit.emplace(fileHash, count);
				}

				replicatorsMap.emplace(replicator, info);
			}
		}
	}

	void DriveEntrySerializer::Save(const DriveEntry& driveEntry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

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
		io::Write16(output, driveEntry.replicas());
		io::Write16(output, driveEntry.minReplicators());
		io::Write8(output, driveEntry.percentApprovers());

		SaveFiles(output, driveEntry.files());
		SaveReplicators(output, driveEntry.replicators());
	}

	DriveEntry DriveEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of DriveEntry", version);

		Key key;
		input.read(key);
		state::DriveEntry entry(key);

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
		entry.setReplicas(io::Read16(input));
		entry.setMinReplicators(io::Read16(input));
		entry.setPercentApprovers(io::Read8(input));

		LoadFiles(input, entry.files());
		LoadReplicators(input, entry.replicators());

		return entry;
	}
}}
