/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NemesisConfiguration.h"
#include "catapult/crypto/KeyPair.h"
#include "sdk/src/extensions/IdGenerator.h"
#include "plugins/txes/mosaic/src/model/MosaicIdGenerator.h"
#include "plugins/txes/namespace/src/model/NamespaceIdGenerator.h"
#include <utility>

namespace catapult { namespace test {

	namespace {
		constexpr auto Max_Height = Height(std::numeric_limits<BlockDuration::ValueType>::max());

		auto IsRoot(const std::string& namespaceName) {
			return std::string::npos == namespaceName.find('.');
		}

		auto CreateRoot(const Key& owner, const std::string& namespaceName) {
			auto id = model::GenerateRootNamespaceId(namespaceName);
			return state::RootNamespace(id, owner, state::NamespaceLifetime(Height(1), Max_Height));
		}

		auto CreateMosaicEntry(
				const MosaicData& mosaic,
				const Key& owner,
				MosaicNonce mosaicNonce) {
			model::MosaicProperties::PropertyValuesContainer values{};
			values[utils::to_underlying_type(model::MosaicPropertyId::Divisibility)] = mosaic.Divisibility;
			values[utils::to_underlying_type(model::MosaicPropertyId::Duration)] = mosaic.Duration;

			model::MosaicFlags flags(model::MosaicFlags::None);
			if (mosaic.IsTransferable)
				flags |= model::MosaicFlags::Transferable;

			if (mosaic.IsSupplyMutable)
				flags |= model::MosaicFlags::Supply_Mutable;

			values[utils::to_underlying_type(model::MosaicPropertyId::Flags)] = utils::to_underlying_type(flags);
			state::MosaicDefinition definition(Height(1), owner, 1, model::MosaicProperties::FromValues(values));

			auto entry = state::MosaicEntry(model::GenerateMosaicId(owner, std::move(mosaicNonce)), definition);
			entry.increaseSupply(Amount(mosaic.Supply));
			return entry;
		}
	}

	NemesisConfiguration::NemesisConfiguration(
			model::NetworkIdentifier networkIdentifier,
			const GenerationHash& nemesisGenerationHash,
			crypto::KeyPair&& nemesisSignerKeyPair,
			uint32_t nemesisAccountVersion,
			std::string binDirectory,
			const std::vector<std::string>& namespaces,
			const std::vector<MosaicData>& mosaics,
			const std::vector<crypto::KeyPair>& harvesterKeyPairs)
		: NetworkIdentifier(networkIdentifier)
		, NemesisGenerationHash(nemesisGenerationHash)
		, NemesisSignerKeyPair(std::move(nemesisSignerKeyPair))
		, NemesisAccountVersion(nemesisAccountVersion)
		, BinDirectory(std::move(binDirectory))
		, HarvesterKeyPairs(harvesterKeyPairs) {
		createNamespaces(namespaces);
		createMosaics(mosaics);
	}

	void NemesisConfiguration::createNamespaces(const std::vector<std::string>& names) {
		const auto& owner = NemesisSignerKeyPair.publicKey();
		for (const auto& namespaceName : names) {
			if (IsRoot(namespaceName)) {
				auto root = CreateRoot(owner, namespaceName);
				RootNamespaces.emplace(root.id(), root);
				NamespaceNames.emplace(root.id(), namespaceName);
				continue;
			}

			auto child = state::Namespace(extensions::GenerateNamespacePath(namespaceName));
			RootNamespaces.find(child.rootId())->second.add(child);
			NamespaceNames.emplace(child.id(), namespaceName);
		}
	}

	void NemesisConfiguration::createMosaics(const std::vector<MosaicData>& mosaics) {
		const auto& owner = NemesisSignerKeyPair.publicKey();
		uint32_t mosaicNonce = 0;
		for (const auto& mosaic : mosaics) {
			auto mosaicEntry = CreateMosaicEntry(mosaic, owner, MosaicNonce(mosaicNonce));
			++mosaicNonce;
			MosaicEntries.emplace(mosaic.Name, mosaicEntry);
			for (const auto& [address, amount] : mosaic.Distribution)
				NemesisAddressToMosaicSeeds[address].emplace_back(MosaicSeed{ mosaic.Name, amount });
		}
	}
}}
