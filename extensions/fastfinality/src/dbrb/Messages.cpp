/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Messages.h"
#include "catapult/ionet/NetworkNode.h"

namespace catapult { namespace dbrb {

	namespace {
		auto PackView(const View& view, uint32_t& payloadSize) {
			payloadSize += sizeof(uint32_t) + utils::checked_cast<size_t, uint32_t>(sizeof(MembershipChanges) * view.Data.size());
			std::vector<model::UniqueEntityPtr<ionet::NetworkNode>> nodes;
			nodes.reserve(view.Data.size());
			for (const auto& pair : view.Data) {
				auto pNode = ionet::PackNode(pair.first);
				payloadSize += pNode->Size;
				nodes.push_back(std::move(pNode));
			}

			return nodes;
		}

		void CopyView(uint8_t*& pData, const View& view, const std::vector<model::UniqueEntityPtr<ionet::NetworkNode>>& nodes) {
			uint32_t count = utils::checked_cast<size_t, uint32_t>(view.Data.size());
			memcpy(pData, &count, sizeof(uint32_t));
			pData += sizeof(uint32_t);
			for (auto i = 0u; i < count; ++i) {
				const auto* pNode = nodes[i].get();
				memcpy(pData, pNode, pNode->Size);
				pData += pNode->Size;
				pData[0] = utils::to_underlying_type(view.Data[i].second);
				pData++;
			}
		}

		auto UnpackView(const uint8_t*& pData) {
			View view;
			auto count = *reinterpret_cast<const uint32_t*>(pData);
			pData += sizeof(uint32_t);
			for (auto i = 0u; i < count; ++i) {
				const auto& networkNode = *reinterpret_cast<const ionet::NetworkNode*>(pData);
				auto node = ionet::UnpackNode(networkNode);
				pData += networkNode.Size;
				auto change = static_cast<MembershipChanges>(pData[0]);
				pData++;
				view.Data.emplace_back(std::make_pair(node, change));
			}

			return view;
		}

#pragma pack(push, 1)

		struct MessagePacket : public ionet::Packet {
			ProcessId Sender;
		};

		struct ReconfigMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Reconfig_Message;

			dbrb::ProcessId ProcessId;
			MembershipChanges MembershipChange;
		};

		struct ReconfigConfirmMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Reconfig_Confirm_Message;
		};

		struct ProposeMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Propose_Message;

			uint32_t ProposedSequenceSize;
		};

		struct ConvergedMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Converged_Message;

			uint32_t ConvergedSequenceSize;
		};

		struct InstallMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Install_Message;

			uint32_t ConvergedSequenceSize;
		};

		struct PrepareMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Prepare_Message;

			uint32_t PayloadSize;
		};

		struct AcknowledgedMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Acknowledged_Message;

			uint32_t PayloadSize;
			catapult::Signature Signature;
		};

		struct CommitMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Commit_Message;

			uint32_t PayloadSize;
			uint32_t CertificateSize;
		};

		struct StateUpdateMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_State_Update_Message;

			uint32_t StateAcknowledgeablePayloadSize;
		};

		struct DeliverMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Deliver_Message;

			uint32_t PayloadSize;
		};

#pragma pack(pop)
	}

	std::unique_ptr<Message> NetworkPacketConverter::toMessage(const ionet::Packet& packet) const {
		const auto* pHandler = findConverter(packet);
		if (!pHandler)
			CATAPULT_THROW_RUNTIME_ERROR_1("packet converter not registered", packet.Type)

		CATAPULT_LOG(trace) << "processing " << packet;
		return (*pHandler)(packet);
	}

	void NetworkPacketConverter::registerConverter(ionet::PacketType type, const PacketConverter& converter) {
		auto rawType = utils::to_underlying_type(type);
		if (rawType >= m_converters.size())
			m_converters.resize(rawType + 1);

		if (m_converters[rawType])
			CATAPULT_THROW_RUNTIME_ERROR_1("converter for type is already registered", type)

		m_converters[rawType] = converter;
	}

	const NetworkPacketConverter::PacketConverter* NetworkPacketConverter::findConverter(const ionet::Packet& packet) const {
		auto rawType = utils::to_underlying_type(packet.Type);
		if (rawType >= m_converters.size()) {
			CATAPULT_LOG(error) << "requested unknown converter: " << packet;
			return nullptr;
		}

		const auto& converter = m_converters[rawType];
		return converter ? &converter : nullptr;
	}

	NetworkPacketConverter::NetworkPacketConverter() {
		registerConverter(ionet::PacketType::Dbrb_Reconfig_Message, [](const ionet::Packet& packet) {
			const auto pMessagePacket = ionet::CoercePacket<ReconfigMessagePacket>(&packet);
			auto pData = reinterpret_cast<const uint8_t*>(pMessagePacket + 1);

			return std::make_unique<ReconfigMessage>(pMessagePacket->Sender, pMessagePacket->ProcessId, pMessagePacket->MembershipChange, UnpackView(pData));
		});

		registerConverter(ionet::PacketType::Dbrb_Reconfig_Confirm_Message, [](const ionet::Packet& packet) {
			const auto pMessagePacket = ionet::CoercePacket<ReconfigConfirmMessagePacket>(&packet);
			auto pData = reinterpret_cast<const uint8_t*>(pMessagePacket + 1);

			return std::make_unique<ReconfigConfirmMessage>(pMessagePacket->Sender, UnpackView(pData));
		});

		registerConverter(ionet::PacketType::Dbrb_Propose_Message, [](const ionet::Packet& packet) {
			const auto pMessagePacket = ionet::CoercePacket<ProposeMessagePacket>(&packet);
			auto pData = reinterpret_cast<const uint8_t*>(pMessagePacket + 1);
			View replacedView = UnpackView(pData);
			Sequence proposedSequence;
			for (auto i = 0u; i < pMessagePacket->ProposedSequenceSize; ++i)
				proposedSequence.tryAppend(UnpackView(pData));

			return std::make_unique<ProposeMessage>(pMessagePacket->Sender, proposedSequence, replacedView);
		});

		registerConverter(ionet::PacketType::Dbrb_Converged_Message, [](const ionet::Packet& packet) {
			const auto pMessagePacket = ionet::CoercePacket<ConvergedMessagePacket>(&packet);
			auto pData = reinterpret_cast<const uint8_t*>(pMessagePacket + 1);
			View replacedView = UnpackView(pData);
			Sequence convergedSequence;
			for (auto i = 0u; i < pMessagePacket->ConvergedSequenceSize; ++i)
				convergedSequence.tryAppend(UnpackView(pData));

			return std::make_unique<ConvergedMessage>(pMessagePacket->Sender, convergedSequence, replacedView);
		});

		registerConverter(ionet::PacketType::Dbrb_Install_Message, [](const ionet::Packet& packet) {
			const auto pMessagePacket = ionet::CoercePacket<InstallMessagePacket>(&packet);
			auto pData = reinterpret_cast<const uint8_t*>(pMessagePacket + 1);
			View leastRecentView = UnpackView(pData);
			View replacedView = UnpackView(pData);
			Sequence convergedSequence;
			for (auto i = 0u; i < pMessagePacket->ConvergedSequenceSize; ++i)
				convergedSequence.tryAppend(UnpackView(pData));

			return std::make_unique<InstallMessage>(pMessagePacket->Sender, leastRecentView, convergedSequence, replacedView);
		});

		registerConverter(ionet::PacketType::Dbrb_Prepare_Message, [](const ionet::Packet& packet) {
			const auto pMessagePacket = ionet::CoercePacket<PrepareMessagePacket>(&packet);
			auto pData = reinterpret_cast<const uint8_t*>(pMessagePacket + 1);
			auto payload = ionet::CreateSharedPacket<ionet::Packet>(pMessagePacket->PayloadSize);
			memcpy(payload.get(), pData, pMessagePacket->PayloadSize);
			pData += pMessagePacket->PayloadSize;

			return std::make_unique<PrepareMessage>(pMessagePacket->Sender, payload, UnpackView(pData));
		});

		registerConverter(ionet::PacketType::Dbrb_Acknowledged_Message, [](const ionet::Packet& packet) {
			const auto pMessagePacket = ionet::CoercePacket<AcknowledgedMessagePacket>(&packet);
			auto pData = reinterpret_cast<const uint8_t*>(pMessagePacket + 1);
			auto payload = ionet::CreateSharedPacket<ionet::Packet>(pMessagePacket->PayloadSize);
			memcpy(payload.get(), pData, pMessagePacket->PayloadSize);
			pData += pMessagePacket->PayloadSize;

			return std::make_unique<AcknowledgedMessage>(pMessagePacket->Sender, payload, UnpackView(pData), pMessagePacket->Signature);
		});

		registerConverter(ionet::PacketType::Dbrb_Commit_Message, [](const ionet::Packet& packet) {
			const auto pMessagePacket = ionet::CoercePacket<CommitMessagePacket>(&packet);
			auto pData = reinterpret_cast<const uint8_t*>(pMessagePacket + 1);
			auto payload = ionet::CreateSharedPacket<ionet::Packet>(pMessagePacket->PayloadSize);
			memcpy(payload.get(), pData, pMessagePacket->PayloadSize);
			pData += pMessagePacket->PayloadSize;

			std::set<Signature> certificate;
			for (auto i = 0u; i < pMessagePacket->CertificateSize; ++i) {
				Signature signature;
				memcpy(signature.data(), pData, Signature_Size);
				pData += Signature_Size;
				certificate.emplace(signature);
			}

			return std::make_unique<CommitMessage>(pMessagePacket->Sender, payload, certificate, UnpackView(pData), UnpackView(pData));
		});

		registerConverter(ionet::PacketType::Dbrb_State_Update_Message, [](const ionet::Packet& packet) {
		  const auto pMessagePacket = ionet::CoercePacket<StateUpdateMessagePacket>(&packet);
		  auto pData = reinterpret_cast<const uint8_t*>(pMessagePacket + 1);
		  auto payload = ionet::CreateSharedPacket<ionet::Packet>(pMessagePacket->StateAcknowledgeablePayloadSize);
		  memcpy(payload.get(), pData, pMessagePacket->StateAcknowledgeablePayloadSize);
		  pData += pMessagePacket->StateAcknowledgeablePayloadSize;

		  return std::make_unique<StateUpdateMessage>(pMessagePacket->Sender, ProcessState{ payload }, UnpackView(pData));
		});

		registerConverter(ionet::PacketType::Dbrb_Deliver_Message, [](const ionet::Packet& packet) {
			const auto pMessagePacket = ionet::CoercePacket<DeliverMessagePacket>(&packet);
			auto pData = reinterpret_cast<const uint8_t*>(pMessagePacket + 1);
			auto payload = ionet::CreateSharedPacket<ionet::Packet>(pMessagePacket->PayloadSize);
			memcpy(payload.get(), pData, pMessagePacket->PayloadSize);
			pData += pMessagePacket->PayloadSize;

			return std::make_unique<DeliverMessage>(pMessagePacket->Sender, payload, UnpackView(pData));
		});
	}

	std::shared_ptr<ionet::Packet> ReconfigMessage::toNetworkPacket() const {
		uint32_t payloadSize = 0u;
		auto packedView = PackView(View, payloadSize);

		auto pPacket = ionet::CreateSharedPacket<ReconfigMessagePacket>(payloadSize);
		pPacket->Sender = Sender;
		pPacket->ProcessId = ProcessId;
		pPacket->MembershipChange = MembershipChange;

		auto pData = reinterpret_cast<uint8_t*>(pPacket.get() + 1);
		CopyView(pData, View, packedView);

		return pPacket;
	}

	std::shared_ptr<ionet::Packet> ReconfigConfirmMessage::toNetworkPacket() const {
		uint32_t payloadSize = 0u;
		auto packedView = PackView(View, payloadSize);
		auto pPacket = ionet::CreateSharedPacket<ReconfigConfirmMessagePacket>(payloadSize);
		pPacket->Sender = Sender;

		auto pData = reinterpret_cast<uint8_t*>(pPacket.get() + 1);
		CopyView(pData, View, packedView);

		return pPacket;
	}

	std::shared_ptr<ionet::Packet> ProposeMessage::toNetworkPacket() const {
		uint32_t payloadSize = 0u;
		std::vector<const View*> views;
		std::vector<std::vector<model::UniqueEntityPtr<ionet::NetworkNode>>> packedViews;

		views.reserve(ProposedSequence.data().size() + 1u);
		packedViews.reserve(views.size());

		views.emplace_back(&ReplacedView);
		packedViews.emplace_back(PackView(ReplacedView, payloadSize));

		for (const auto& view : ProposedSequence.data()) {
			views.emplace_back(&view);
			packedViews.emplace_back(PackView(view, payloadSize));
		}

		auto pPacket = ionet::CreateSharedPacket<ProposeMessagePacket>(payloadSize);
		pPacket->Sender = Sender;
		pPacket->ProposedSequenceSize = utils::checked_cast<size_t, uint32_t>(ProposedSequence.data().size());

		auto pData = reinterpret_cast<uint8_t*>(pPacket.get() + 1);
		for (auto i = 0u; i < views.size(); ++i)
			CopyView(pData, *views[i], packedViews[i]);

		return pPacket;
	}

	std::shared_ptr<ionet::Packet> ConvergedMessage::toNetworkPacket() const {
		uint32_t payloadSize = 0u;
		std::vector<const View*> views;
		std::vector<std::vector<model::UniqueEntityPtr<ionet::NetworkNode>>> packedViews;

		views.reserve(ConvergedSequence.data().size() + 1u);
		packedViews.reserve(views.size());

		views.emplace_back(&ReplacedView);
		packedViews.emplace_back(PackView(ReplacedView, payloadSize));

		for (const auto& view : ConvergedSequence.data()) {
			views.emplace_back(&view);
			packedViews.emplace_back(PackView(view, payloadSize));
		}

		auto pPacket = ionet::CreateSharedPacket<ConvergedMessagePacket>(payloadSize);
		pPacket->Sender = Sender;
		pPacket->ConvergedSequenceSize = utils::checked_cast<size_t, uint32_t>(ConvergedSequence.data().size());

		auto pData = reinterpret_cast<uint8_t*>(pPacket.get() + 1);
		for (auto i = 0u; i < views.size(); ++i)
			CopyView(pData, *views[i], packedViews[i]);

		return pPacket;
	}

	std::shared_ptr<ionet::Packet> InstallMessage::toNetworkPacket() const {
		uint32_t payloadSize = 0u;
		std::vector<const View*> views;
		std::vector<std::vector<model::UniqueEntityPtr<ionet::NetworkNode>>> packedViews;

		views.reserve(ConvergedSequence.data().size() + 2u);
		packedViews.reserve(views.size());

		views.emplace_back(&LeastRecentView);
		packedViews.emplace_back(PackView(LeastRecentView, payloadSize));

		views.emplace_back(&ReplacedView);
		packedViews.emplace_back(PackView(ReplacedView, payloadSize));

		for (const auto& view : ConvergedSequence.data()) {
			views.emplace_back(&view);
			packedViews.emplace_back(PackView(view, payloadSize));
		}

		auto pPacket = ionet::CreateSharedPacket<InstallMessagePacket>(payloadSize);
		pPacket->Sender = Sender;

		auto pData = reinterpret_cast<uint8_t*>(pPacket.get() + 1);
		for (auto i = 0u; i < views.size(); ++i)
			CopyView(pData, *views[i], packedViews[i]);

		return pPacket;
	}

	std::shared_ptr<ionet::Packet> PrepareMessage::toNetworkPacket() const {
		uint32_t payloadSize = Payload->Size;
		auto packedView = PackView(View, payloadSize);
		auto pPacket = ionet::CreateSharedPacket<PrepareMessagePacket>(payloadSize);
		pPacket->Sender = Sender;
		pPacket->PayloadSize = Payload->Size;

		auto pData = reinterpret_cast<uint8_t*>(pPacket.get() + 1);
		memcpy(pData, Payload.get(), Payload->Size);

		CopyView(pData, View, packedView);

		return pPacket;
	}

	std::shared_ptr<ionet::Packet> AcknowledgedMessage::toNetworkPacket() const {
		uint32_t payloadSize = Payload->Size;
		auto packedView = PackView(View, payloadSize);
		auto pPacket = ionet::CreateSharedPacket<AcknowledgedMessagePacket>(payloadSize);
		pPacket->Sender = Sender;
		pPacket->Signature = Signature;
		pPacket->PayloadSize = Payload->Size;

		auto pData = reinterpret_cast<uint8_t*>(pPacket.get() + 1);
		memcpy(pData, Payload.get(), Payload->Size);

		CopyView(pData, View, packedView);

		return pPacket;
	}

	std::shared_ptr<ionet::Packet> CommitMessage::toNetworkPacket() const {
		uint32_t certificateSize = utils::checked_cast<size_t, uint32_t>(Certificate.size());
		uint32_t payloadSize = Payload->Size + utils::checked_cast<size_t, uint32_t>(Signature_Size * certificateSize);
		auto packedCertificateView = PackView(CertificateView, payloadSize);
		auto packedCurrentView = PackView(CurrentView, payloadSize);
		auto pPacket = ionet::CreateSharedPacket<CommitMessagePacket>(payloadSize);
		pPacket->Sender = Sender;
		pPacket->PayloadSize = Payload->Size;
		pPacket->CertificateSize = certificateSize;

		auto pData = reinterpret_cast<uint8_t*>(pPacket.get() + 1);
		memcpy(pData, Payload.get(), Payload->Size);

		for (const auto& signature : Certificate) {
			memcpy(pData, signature.data(), Signature_Size);
			pData += Signature_Size;
		}

		CopyView(pData, CertificateView, packedCertificateView);
		CopyView(pData, CurrentView, packedCurrentView);

		return pPacket;
	}

	std::shared_ptr<ionet::Packet> StateUpdateMessage::toNetworkPacket() const {
		uint32_t payloadSize = State.AcknowledgeablePayload->Size;
		auto packedView = PackView(PendingChanges, payloadSize);
		auto pPacket = ionet::CreateSharedPacket<StateUpdateMessagePacket>(payloadSize);
		pPacket->Sender = Sender;
		pPacket->StateAcknowledgeablePayloadSize = State.AcknowledgeablePayload->Size;

		auto pData = reinterpret_cast<uint8_t*>(pPacket.get() + 1);
		memcpy(pData, State.AcknowledgeablePayload.get(), State.AcknowledgeablePayload->Size);

		CopyView(pData, PendingChanges, packedView);

		return pPacket;
	}

	std::shared_ptr<ionet::Packet> DeliverMessage::toNetworkPacket() const {
		uint32_t payloadSize = Payload->Size;
		auto packedView = PackView(View, payloadSize);
		auto pPacket = ionet::CreateSharedPacket<DeliverMessagePacket>(payloadSize);
		pPacket->Sender = Sender;
		pPacket->PayloadSize = Payload->Size;

		auto pData = reinterpret_cast<uint8_t*>(pPacket.get() + 1);
		memcpy(pData, Payload.get(), Payload->Size);

		CopyView(pData, View, packedView);

		return pPacket;
	}
}}