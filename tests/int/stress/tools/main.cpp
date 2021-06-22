/**
*** Copyright (c) 2018-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "catapult/cache_core/AccountStateCacheSerializers.h"
#include "catapult/crypto/KeyUtils.h"
#include "catapult/model/Address.h"
#include "catapult/state/AccountState.h"
#include "catapult/utils/HexParser.h"
#include "catapult/utils/HexParser.h"
#include "catapult/types.h"
#include "sdk/src/extensions/IdGenerator.h"
#include "tests/int/stress/test/InputDependentTest.h"
#include "tools/tools/ToolMain.h"
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <random>
#include <catapult/tree/MemoryDataSource.h>
#include "tests/catapult/cache/test/PatriciaTreeTestUtils.h"
// Short alias for this namespace
namespace pt = boost::property_tree;

namespace catapult { namespace tools { namespace address {

	namespace {

		// indexes into parsed line
		enum : size_t { Placeholder, Address, AccountState };

		std::pair<state::AccountState, bool> ParseAccount(const std::vector<std::string>& parts) {
			auto address = model::StringToAddress(parts[Address]);

			std::vector<uint8_t> serializedBuffer(parts[AccountState].size() / 2);
			utils::ParseHexStringIntoContainer(parts[AccountState].data(), parts[AccountState].size(), serializedBuffer);
			auto accountState = cache::AccountStatePrimarySerializer::DeserializeValue(serializedBuffer);

			return std::make_pair(accountState, address == accountState.Address);
		}

		std::vector<std::string> StringifyAccount(const state::AccountState& accountState) {
			std::string strAccount = crypto::FormatKeyAsString(cache::AccountStatePrimarySerializer::SerializeValue(accountState));

			return { "" , model::AddressToString(accountState.Address) , strAccount };
		}

		enum  Mode {
			JsonToBinary,
			BinaryToJson,
			CalculateBinaryRoot
		};

		class PatriciaTreeGeneratorTool : public Tool {
		public:
			std::string name() const override {
				return "Patricia Tree Generator Tool";
			}

			void prepareOptions(OptionsBuilder& optionsBuilder, OptionsPositional&) override {
				optionsBuilder("input,i",
							   OptionsValue<std::string>(m_inputpath)->default_value("../tests/int/stress/resources/1.patricia-tree-account.dat"),
							   "path to input file");
				optionsBuilder("gtest_color",
							   OptionsValue<std::string>(m_inputpath)->default_value("no"),
							   "no op");
				optionsBuilder("output,o",
							   OptionsValue<std::string>(m_outputpath)->default_value("../tests/int/stress/resources/accounts.json"),
							   "path to output file");
				optionsBuilder("mode,m",
							   boost::program_options::value<std::string>()->default_value("JsonToBinary"),
							   "mode { JsonToBinary, BinaryToJson, CalculateBinaryRoot }");
			}

			int run(const Options& options) override {
				std::string mode = options["mode"].as<std::string>();
				if (!parseEnum(mode))
					CATAPULT_THROW_INVALID_ARGUMENT_1("unknown mode", mode);

				switch (m_mode) {
					case JsonToBinary: {
						createBinaryFromJson();
						break;
					}
					case BinaryToJson: {
						createJsonFromBinary();
						break;
					}
					case CalculateBinaryRoot: {
						calculateMerkleRoot();
						break;
					}
				}

				return 0;
			}

		private:

			void createJsonFromBinary() {
				pt::ptree root, children;
				test::RunInputDependentTest(m_inputpath, ParseAccount, [&children](const state::AccountState& accountState) {
					pt::ptree accountJson;

					accountJson.put("Address", model::AddressToString(accountState.Address));
					accountJson.put("AddressHeight", accountState.AddressHeight);
					accountJson.put("PublicKeyHeight", accountState.PublicKeyHeight);
					accountJson.put("PublicKey", crypto::FormatKeyAsString(accountState.PublicKey));
					accountJson.put("AccountType", (uint64_t)accountState.AccountType);
					accountJson.put("OptimizedMosaicId", accountState.Balances.optimizedMosaicId().unwrap());
					accountJson.put("TrackedMosaicId", accountState.Balances.trackedMosaicId().unwrap());
					if(HasFlag(state::AccountPublicKeys::KeyType::Linked, accountState.SupplementalPublicKeys.mask()))
					{
						accountJson.put("LinkedAccountKey", crypto::FormatKeyAsString(GetLinkedPublicKey(accountState)));
					}
					if(HasFlag(state::AccountPublicKeys::KeyType::Node, accountState.SupplementalPublicKeys.mask()))
					{
				  		accountJson.put("LinkedNodeKey", crypto::FormatKeyAsString(GetNodePublicKey(accountState)));
					}

					pt::ptree mosaics;
					for (auto& pair : accountState.Balances) {
						pt::ptree mosaic;
						mosaic.put("MosaicId", pair.first.unwrap());
						mosaic.put("Amount", pair.second);

						mosaics.push_back({ "", mosaic });
					}

					accountJson.put_child("mosaics", mosaics);

					pt::ptree snapshots;
					for (auto& pair : accountState.Balances.snapshots()) {
						pt::ptree snapshot;
						snapshot.put("Amount", pair.Amount.unwrap());
						snapshot.put("Height", pair.BalanceHeight.unwrap());

						snapshots.push_back({ "", snapshot });
					}

					accountJson.put_child("snapshots", snapshots);

					children.push_back({ "", accountJson });
				});
				root.add_child("accounts", children);
				pt::write_json(m_outputpath, root);
			}
			/// A memory patricia tree used in cache tests.
			using MemoryAccountPatriciaTree = tree::PatriciaTree<
					cache::SerializerHashedKeyEncoder<cache::AccountStatePatriciaTreeSerializer>,
					tree::MemoryDataSource>;

			Hash256 CalculateRootHash(const std::vector<std::pair<catapult::Address, state::AccountState>>& pairs) {
				tree::MemoryDataSource dataSource;
				MemoryAccountPatriciaTree tree(dataSource);

				for (const auto& pair : pairs)
					tree.set(pair.first, pair.second);

				return tree.root();
			}
			void calculateMerkleRoot() {
				std::vector<std::pair<catapult::Address , state::AccountState>> accounts;
				test::RunInputDependentTest(m_inputpath, ParseAccount, [&accounts](const state::AccountState& accountState) {
					accounts.push_back(std::pair(accountState.Address, accountState));
				});
				auto hash = CalculateRootHash(accounts);
				CATAPULT_LOG(info) << crypto::FormatKeyAsString(*reinterpret_cast<Key*>(&hash));
			}
			void createBinaryFromJson() {
				// Load the json file in this ptree
				pt::ptree root;
				pt::read_json(m_inputpath, root);
				auto children = root.get_child("accounts");

				std::ofstream output;
				output.open(m_outputpath, std::ofstream::out);

				if (!output){
					CATAPULT_THROW_INVALID_ARGUMENT_1("can't open file", m_outputpath);
					output.close();
				}

				output << "# : account address : serialized account state" << std::endl;

				for (pt::ptree::value_type &accountJson : children) {
					auto& account = accountJson.second;

					state::AccountState accountState(
							model::StringToAddress(account.get<std::string>("Address")),
							Height(account.get<uint64_t>("AddressHeight"))
					);
					accountState.PublicKeyHeight = Height(account.get<uint64_t>("PublicKeyHeight"));
					accountState.PublicKey = crypto::ParseKey(account.get<std::string>("PublicKey"));
					accountState.AccountType = (state::AccountType)account.get<uint8_t>("AccountType");
					accountState.AccountType = (state::AccountType)account.get<uint8_t>("AccountType");
					accountState.Balances.optimize(MosaicId(account.get<uint64_t>("OptimizedMosaicId")));
					accountState.Balances.track(MosaicId(account.get<uint64_t>("TrackedMosaicId")));
					auto linkedKey = account.get_optional<std::string>("LinkedAccountKey");
					auto nodeKey = account.get_optional<std::string>("LinkedNodeKey");
					if(linkedKey.has_value())
					{
						accountState.SupplementalPublicKeys.linked().unset();
						accountState.SupplementalPublicKeys.linked().set(crypto::ParseKey(linkedKey.value()));
					}
					if(nodeKey.has_value())
					{
						accountState.SupplementalPublicKeys.node().unset();
						accountState.SupplementalPublicKeys.node().set(crypto::ParseKey(nodeKey.value()));
					}


					for (pt::ptree::value_type&  mosaicJson: account.get_child("mosaics")) {
						auto& mosaic = mosaicJson.second;
						accountState.Balances.credit(
								MosaicId(mosaic.get<std::uint64_t>("MosaicId")),
								Amount(mosaic.get<uint64_t>("Amount"))
						);
					}

					for (pt::ptree::value_type&  snapshotJson: account.get_child("snapshots")) {
						auto& snapshot = snapshotJson.second;
						accountState.Balances.addSnapshot({
							Amount(snapshot.get<uint64_t>("Amount")),
							Height(snapshot.get<uint64_t>("Height"))
						});
					}

					auto parts = StringifyAccount(accountState);
					output << parts[Placeholder] << ": " << parts[Address] << " : " << parts[AccountState] << std::endl;
				}

				output.close();
			}

			bool parseEnum(const std::string& mode) {
				std::map<std::string, Mode> table = {
						{ "binarytojson", Mode::BinaryToJson },
						{ "jsontobinary", Mode::JsonToBinary },
						{ "calculatebinaryroot", Mode::CalculateBinaryRoot }
				};

				std::string lower(mode);
				std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

				if (!table.count(lower))
					return false;

				m_mode = table[lower];

				return true;
			}

		private:
			std::string m_inputpath;
			std::string m_outputpath;
			Mode m_mode;
		};
	}
}}}

int main(int argc, const char** argv) {
	catapult::tools::address::PatriciaTreeGeneratorTool tool;
	return catapult::tools::ToolMain(argc, argv, tool);
}
