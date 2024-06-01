/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/crypto/KeyPair.h"
#include "catapult/model/NetworkInfo.h"
#include "catapult/utils/Hashers.h"
#include "plugins/txes/mosaic/src/state/MosaicEntry.h"
#include "plugins/txes/namespace/src/state/RootNamespace.h"
#include "plugins/txes/namespace/src/model/NamespaceConstants.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace test {

	/// A mosaic seed.
	struct MosaicSeed {
		/// Mosaic name.
		std::string Name;

		/// Mosaic amount.
		catapult::Amount Amount;
	};

	/// A mosaic seed.
	struct MosaicData {
		/// Mosaic name.
		std::string Name;

		/// Mosaic supply.
		uint64_t Supply;

		/// Mosaic divisibility.
		uint8_t Divisibility;

		/// Mosaic duration.
		uint64_t Duration;

		/// True if mosaic is transferable, false otherwise.
		bool IsTransferable;

		/// True if mosaic supply is mutable, false otherwise.
		bool IsSupplyMutable;

		/// Mosaic distrubution.
		std::vector<std::pair<UnresolvedAddress, Amount>> Distribution;
	};

	using NamespaceIdToNameMap = std::unordered_map<NamespaceId, std::string, utils::BaseValueHasher<NamespaceId>>;
	using NamespaceIdToRootNamespaceMap = std::unordered_map<NamespaceId, state::RootNamespace, utils::BaseValueHasher<NamespaceId>>;
	using MosaicNameToMosaicEntryMap = std::unordered_map<std::string, state::MosaicEntry>;
	using AddressToMosaicSeedsMap = std::unordered_map<UnresolvedAddress, std::vector<MosaicSeed>, utils::ArrayHasher<UnresolvedAddress>>;

	/// Nemesis configuration.
	struct NemesisConfiguration {
	public:
		explicit NemesisConfiguration(
			model::NetworkIdentifier networkIdentifier,
			const GenerationHash& nemesisGenerationHash,
			crypto::KeyPair&& nemesisSignerKeyPair,
			uint32_t nemesisAccountVersion,
			std::string binDirectory,
			const std::vector<std::string>& namespaces,
			const std::vector<MosaicData>& mosaics,
			const std::vector<crypto::KeyPair>& harvesterKeyPairs);

	public:
		/// Block chain network identifier.
		model::NetworkIdentifier NetworkIdentifier;

		/// Nemesis generation hash.
		GenerationHash NemesisGenerationHash;

		/// Nemesis signer private key.
		crypto::KeyPair NemesisSignerKeyPair;

		/// Version of the nemesis account
		uint32_t NemesisAccountVersion;

		/// Binary destination directory.
		std::string BinDirectory;

		/// Map containing all namespace names.
		NamespaceIdToNameMap NamespaceNames;

		/// Map containing all root namespace names.
		NamespaceIdToRootNamespaceMap RootNamespaces;

		/// Map containing all mosaic entries.
		MosaicNameToMosaicEntryMap MosaicEntries;

		/// Map of nemesis account addresses to mosaic seeds.
		AddressToMosaicSeedsMap NemesisAddressToMosaicSeeds;

		/// Private keys of initial harvesters.
		const std::vector<crypto::KeyPair>& HarvesterKeyPairs;

	private:
		void createNamespaces(const std::vector<std::string>& names);
		void createMosaics(const std::vector<MosaicData>& mosaics);
	};
}}
