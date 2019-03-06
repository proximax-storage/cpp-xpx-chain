/**
*** Copyright (c) 2016-present,
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

#include "src/extensions/RemoteDiagnosticApi.h"
#include "catapult/handlers/DiagnosticHandlers.h"
#include "catapult/ionet/Packet.h"
#include "tests/test/other/RemoteApiTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace extensions {

	namespace {
		// region ConfirmTimestampedHashesTraits

		struct ConfirmTimestampedHashesTraits {
			using RequestParamType = state::TimestampedHashRange;
			using ResponseType = state::TimestampedHashRange;

			static constexpr auto PacketType() { return ionet::PacketType::Confirm_Timestamped_Hashes; }
			static constexpr auto Request_Entity_Size = sizeof(state::TimestampedHash);
			static constexpr auto Response_Entity_Size = Request_Entity_Size;

			static auto CreateResponsePacket(uint32_t numTimestampedHashes) {
				uint32_t payloadSize = numTimestampedHashes * Response_Entity_Size;
				auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(payloadSize);
				test::FillWithRandomData({ pPacket->Data(), payloadSize });

				auto pData = pPacket->Data();
				for (auto i = 0u; i < numTimestampedHashes; ++i, pData += Response_Entity_Size) {
					auto& timestampedHash = reinterpret_cast<state::TimestampedHash&>(*pData);
					timestampedHash.Time = Timestamp(5 * i);
					test::FillWithRandomData(timestampedHash.Hash);
				}

				return pPacket;
			}

			static std::vector<state::TimestampedHash> RequestParamValues() {
				return {
					state::TimestampedHash(Timestamp(12), { { 123 } }),
					state::TimestampedHash(Timestamp(23), { { 234 } }),
					state::TimestampedHash(Timestamp(34), { { 213 } })
				};
			}

			static auto Invoke(const RemoteDiagnosticApi& api, RequestParamType&& param) {
				return api.confirmTimestampedHashes(std::move(param));
			}

			static void ValidateResponse(const ionet::Packet& response, const state::TimestampedHashRange& timestampedHashes) {
				ASSERT_EQ(3u, timestampedHashes.size());

				auto pExpectedTimestampedHash = reinterpret_cast<const state::TimestampedHash*>(response.Data());
				auto iter = timestampedHashes.cbegin();
				for (auto i = 0u; i < timestampedHashes.size(); ++i) {
					std::string message = "comparing timestamped hash at " + std::to_string(i);
					const auto& actualTimestampedHash = *iter;
					EXPECT_EQ(*pExpectedTimestampedHash, actualTimestampedHash) << message;
					++pExpectedTimestampedHash;
					++iter;
				}
			}
		};

		// endregion

		// region DiagnosticCountersTraits

		struct DiagnosticCountersTraits {
			static constexpr auto Packet_Type = ionet::PacketType::Diagnostic_Counters;

			static auto Invoke(const RemoteDiagnosticApi& api) {
				return api.diagnosticCounters();
			}

			static auto CreateValidResponsePacket() {
				auto pResponsePacket = ionet::CreateSharedPacket<ionet::Packet>(3 * sizeof(model::DiagnosticCounterValue));
				pResponsePacket->Type = Packet_Type;

				auto* pCounters = reinterpret_cast<model::DiagnosticCounterValue*>(pResponsePacket->Data());
				pCounters[0] = { 123u, 7u };
				pCounters[1] = { 777u, 88u };
				pCounters[2] = { 225u, 22u };
				return pResponsePacket;
			}

			static auto CreateMalformedResponsePacket() {
				// just change the size because no responses are intrinsically invalid
				auto pResponsePacket = CreateValidResponsePacket();
				--pResponsePacket->Size;
				return pResponsePacket;
			}

			static void ValidateRequest(const ionet::Packet& packet) {
				EXPECT_TRUE(ionet::IsPacketValid(packet, Packet_Type));
			}

			static void ValidateResponse(const ionet::Packet&, const model::EntityRange<model::DiagnosticCounterValue>& counterValues) {
				ASSERT_EQ(3u, counterValues.size());

				auto iter = counterValues.cbegin();
				EXPECT_EQ(123u, iter->Id);
				EXPECT_EQ(7u, iter->Value);

				++iter;
				EXPECT_EQ(777u, iter->Id);
				EXPECT_EQ(88u, iter->Value);

				++iter;
				EXPECT_EQ(225u, iter->Id);
				EXPECT_EQ(22u, iter->Value);
			}
		};

		// endregion

		// region ActiveNodeInfosTraits

		struct ActiveNodeInfosTraits {
			static constexpr auto Packet_Type = ionet::PacketType::Active_Node_Infos;
			static constexpr auto Response_Entity_Size = sizeof(ionet::PackedNodeInfo) + 2 * sizeof(ionet::PackedConnectionState);
			static constexpr auto Num_Node_Infos = 3u;

			static auto Invoke(const RemoteDiagnosticApi& api) {
				return api.activeNodeInfos();
			}

			static auto CreateValidResponsePacket() {
				uint32_t payloadSize = Num_Node_Infos * Response_Entity_Size;
				auto pResponsePacket = ionet::CreateSharedPacket<ionet::Packet>(payloadSize);
				pResponsePacket->Type = Packet_Type;
				test::FillWithRandomData({ pResponsePacket->Data(), payloadSize });

				// set sizes appropriately
				auto* pData = pResponsePacket->Data();
				for (auto i = 0u; i < Num_Node_Infos; ++i) {
					auto& nodeInfo = reinterpret_cast<ionet::PackedNodeInfo&>(*pData);
					nodeInfo.Size = Response_Entity_Size;
					nodeInfo.ConnectionStatesCount = 2;
					pData += Response_Entity_Size;
				}

				return pResponsePacket;
			}

			static auto CreateMalformedResponsePacket() {
				// just change the size because no responses are intrinsically invalid
				auto pResponsePacket = CreateValidResponsePacket();
				--pResponsePacket->Size;
				return pResponsePacket;
			}

			static void ValidateRequest(const ionet::Packet& packet) {
				EXPECT_TRUE(ionet::IsPacketValid(packet, Packet_Type));
			}

			static void ValidateResponse(const ionet::Packet& response, const model::EntityRange<ionet::PackedNodeInfo>& nodeInfos) {
				ASSERT_EQ(static_cast<uint32_t>(Num_Node_Infos), nodeInfos.size());

				auto iter = nodeInfos.cbegin();
				const auto* pResponseData = response.Data();
				for (auto i = 0u; i < Num_Node_Infos; ++i) {
					auto expectedSize = Response_Entity_Size;
					auto message = "node info at " + std::to_string(i);

					// Sanity:
					ASSERT_EQ(expectedSize, reinterpret_cast<const ionet::PackedNodeInfo&>(*pResponseData).Size) << message;

					// Assert: check the node info size then the memory
					ASSERT_EQ(expectedSize, iter->Size) << message;
					EXPECT_EQ_MEMORY(pResponseData, &*iter, iter->Size) << message;

					pResponseData += expectedSize;
					++iter;
				}
			}
		};

		// endregion

		template<typename TIdentifier, ionet::PacketType Packet_Type>
		struct InfosTraits {
		public:
			using RequestParamType = model::EntityRange<TIdentifier>;
			using EntityType = model::CacheEntryInfo<TIdentifier>;
			using ResponseType = model::EntityRange<EntityType>;

			static constexpr auto PacketType() { return Packet_Type; }
			static constexpr auto Response_Entity_Data_Size = 10u;
			static constexpr auto Response_Entity_Size = sizeof(EntityType) + Response_Entity_Data_Size;

		public:
			static auto CreateResponsePacket(uint32_t numInfos) {
				uint32_t payloadSize = numInfos * Response_Entity_Size;
				auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(payloadSize);
				test::FillWithRandomData({ pPacket->Data(), payloadSize });

				auto pData = pPacket->Data();
				for (auto i = 0u; i < numInfos; ++i, pData += Response_Entity_Size) {
					auto& info = reinterpret_cast<EntityType&>(*pData);
					info.Size = Response_Entity_Size;
					info.DataSize = Response_Entity_Data_Size;
					info.Id = ToIdentifier(5 * i);
				}

				return pPacket;
			}

			static std::vector<TIdentifier> RequestParamValues() {
				return { ToIdentifier(123), ToIdentifier(234), ToIdentifier(213) };
			}

			static void ValidateResponse(const ionet::Packet& response, const model::EntityRange<EntityType>& infos) {
				ASSERT_EQ(3u, infos.size());

				auto pData = response.Data();
				auto iter = infos.cbegin();
				for (auto i = 0u; i < infos.size(); ++i) {
					std::string message = "comparing info at " + std::to_string(i);
					const auto& expectedInfo = reinterpret_cast<const EntityType&>(*pData);
					const auto& actualInfo = *iter;
					ASSERT_EQ(expectedInfo.Size, actualInfo.Size) << message;
					ASSERT_EQ(expectedInfo.DataSize, actualInfo.DataSize) << message;

					EXPECT_EQ(expectedInfo.Id, actualInfo.Id) << message;
					EXPECT_EQ_MEMORY(expectedInfo.DataPtr(), actualInfo.DataPtr(), actualInfo.DataSize) << message;

					++iter;
					pData += expectedInfo.Size;
				}
			}

		private:
			template<typename X = std::enable_if_t<utils::traits::is_pod<TIdentifier>::value>>
			static TIdentifier ToIdentifier(uint32_t value) {
				return TIdentifier(value);
			}

			static TIdentifier ToIdentifier(uint32_t value) {
				return TIdentifier{ { static_cast<uint8_t>(value) } };
			}
		};

		// region AccountInfosTraits

		struct AccountInfosTraits : public InfosTraits<Address, ionet::PacketType::Account_Infos> {
			static constexpr auto Request_Entity_Size = Address_Decoded_Size;

			static auto Invoke(const RemoteDiagnosticApi& api, RequestParamType&& param) {
				return api.accountInfos(std::move(param));
			}
		};

		// endregion

		// region AccountPropertiesInfosTraits

		struct AccountPropertiesInfosTraits : public InfosTraits<Address, ionet::PacketType::Account_Properties_Infos> {
			static constexpr auto Request_Entity_Size = Address_Decoded_Size;

			static auto Invoke(const RemoteDiagnosticApi& api, RequestParamType&& param) {
				return api.accountPropertiesInfos(std::move(param));
			}
		};

		// endregion

		// region NamespaceInfosTraits

		struct NamespaceInfosTraits : public InfosTraits<NamespaceId, ionet::PacketType::Namespace_Infos> {
			static constexpr auto Request_Entity_Size = sizeof(NamespaceId);

			static auto Invoke(const RemoteDiagnosticApi& api, RequestParamType&& param) {
				return api.namespaceInfos(std::move(param));
			}
		};

		// endregion

		// region MosaicInfosTraits

		struct MosaicInfosTraits : public InfosTraits<MosaicId, ionet::PacketType::Mosaic_Infos> {
			static constexpr auto Request_Entity_Size = sizeof(MosaicId);

			static auto Invoke(const RemoteDiagnosticApi& api, RequestParamType&& param) {
				return api.mosaicInfos(std::move(param));
			}
		};

		// endregion

		// region DiagnosticApiTraits

		template<typename TTraits>
		struct DiagnosticApiTraits {
			static constexpr uint32_t Num_Entities = 3;
			static constexpr uint32_t Request_Data_Size = Num_Entities * TTraits::Request_Entity_Size;

			static auto CreateRequestParam() {
				return TTraits::RequestParamType::CopyFixed(
						reinterpret_cast<uint8_t*>(TTraits::RequestParamValues().data()),
						Num_Entities);
			}

			static auto Invoke(const RemoteDiagnosticApi& api) {
				return TTraits::Invoke(api, CreateRequestParam());
			}

			static auto CreateValidResponsePacket() {
				auto pResponsePacket = TTraits::CreateResponsePacket(Num_Entities);
				pResponsePacket->Type = TTraits::PacketType();
				return pResponsePacket;
			}

			static auto CreateMalformedResponsePacket() {
				// the packet is malformed because it contains a partial entity
				auto pResponsePacket = CreateValidResponsePacket();
				--pResponsePacket->Size;
				return pResponsePacket;
			}

			static void ValidateRequest(const ionet::Packet& packet) {
				EXPECT_EQ(TTraits::PacketType(), packet.Type);
				ASSERT_EQ(sizeof(ionet::Packet) + Request_Data_Size, packet.Size);
				EXPECT_EQ_MEMORY(packet.Data(), TTraits::RequestParamValues().data(), Request_Data_Size);
			}

			static void ValidateResponse(const ionet::Packet& response, const typename TTraits::ResponseType& responseEntities) {
				TTraits::ValidateResponse(response, responseEntities);
			}
		};

		// endregion

		// region RemoteDiagnosticApiTraits

		struct RemoteDiagnosticApiTraits {
			static auto Create(ionet::PacketIo& packetIo) {
				return CreateRemoteDiagnosticApi(packetIo);
			}
		};

		// endregion
	}

	using DiagnosticConfirmTimestampedHashesTraits = DiagnosticApiTraits<ConfirmTimestampedHashesTraits>;
	DEFINE_REMOTE_API_TESTS_EMPTY_RESPONSE_VALID(RemoteDiagnosticApi, DiagnosticConfirmTimestampedHashes)

	DEFINE_REMOTE_API_TESTS_EMPTY_RESPONSE_INVALID(RemoteDiagnosticApi, DiagnosticCounters)
	DEFINE_REMOTE_API_TESTS_EMPTY_RESPONSE_INVALID(RemoteDiagnosticApi, ActiveNodeInfos)

	using DiagnosticAccountInfosTraits = DiagnosticApiTraits<AccountInfosTraits>;
	using DiagnosticAccountPropertiesInfosTraits = DiagnosticApiTraits<AccountPropertiesInfosTraits>;
	using DiagnosticNamespaceInfosTraits = DiagnosticApiTraits<NamespaceInfosTraits>;
	using DiagnosticMosaicInfosTraits = DiagnosticApiTraits<MosaicInfosTraits>;
	DEFINE_REMOTE_API_TESTS_EMPTY_RESPONSE_INVALID(RemoteDiagnosticApi, DiagnosticAccountInfos)
	DEFINE_REMOTE_API_TESTS_EMPTY_RESPONSE_INVALID(RemoteDiagnosticApi, DiagnosticAccountPropertiesInfos)
	DEFINE_REMOTE_API_TESTS_EMPTY_RESPONSE_INVALID(RemoteDiagnosticApi, DiagnosticNamespaceInfos)
	DEFINE_REMOTE_API_TESTS_EMPTY_RESPONSE_INVALID(RemoteDiagnosticApi, DiagnosticMosaicInfos)
}}
