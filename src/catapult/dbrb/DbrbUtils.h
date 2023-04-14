/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma  once
#include "DbrbViewFetcher.h"
#include "catapult/ionet/Packet.h"
#include "catapult/types.h"
#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <optional>
#include <cstdint>

namespace catapult { namespace dbrb {

	using Payload = std::shared_ptr<ionet::Packet>;
	using CertificateType = std::map<ProcessId, Signature>;

	void Write(uint8_t*& pBuffer, const utils::RawBuffer& data);
	void Write(uint8_t*& pBuffer, uint8_t byte);
	struct View;
	void Write(uint8_t*& pBuffer, const View& view);
	void Write(uint8_t*& pBuffer, const Payload& payload);
	void Write(uint8_t*& pBuffer, const CertificateType& certificate);

	template<typename T>
	T Read(const uint8_t*& pBuffer);
	template<>
	ProcessId Read(const uint8_t*& pBuffer);
	template<>
	Signature Read(const uint8_t*& pBuffer);
	template<>
	Hash256 Read(const uint8_t*& pBuffer);
	template<>
	View Read(const uint8_t*& pBuffer);
	template<>
	Payload Read(const uint8_t*& pBuffer);
	template<>
	CertificateType Read(const uint8_t*& pBuffer);

	Hash256 CalculateHash(const std::vector<RawBuffer>& buffers);
	Hash256 CalculatePayloadHash(const Payload& payload);

	std::string ProcessIdToString(const ProcessId& processId);

	/// View of the system.
	struct View {
		/// Set of IDs of current members of the view.
		ViewData Data;

		/// Check if \a processId is a member of this view.
		bool isMember(const ProcessId& processId) const;

		/// Calculate quorum size of this view.
		size_t quorumSize() const;

		/// Calculate the size in bytes required to serialize this view.
		size_t packedSize() const;

		/// Comparison operators; if view A is less recent than view B, then A < B.
		bool operator==(const View& other) const;
		bool operator!=(const View& other) const;
		bool operator<(const View& other) const;
		bool operator>(const View& other) const;
		bool operator<=(const View& other) const;
		bool operator>=(const View& other) const;

		/// Insertion operator for outputting \a view to \a out.
		friend std::ostream& operator<<(std::ostream& out, const View& view) {
			bool leadingSpace = false;

			out << "[";
			for (const auto& processId : view.Data) {
				if (leadingSpace)
					out << " ";

				out << ProcessIdToString(processId);
				leadingSpace = true;
			}
			out << "]";

			return out;
		}

		/// Merge other view into this view.
		View& merge(const View& other);

		/// Remove from this view's \a Data all changes that appear in the other view.
		View& difference(const View& other);

		/// Merge two views into one.
		static View merge(const View& a, const View& b) {
			ViewData newData;
			for (const auto& change : a.Data)
				newData.insert(change);
			for (const auto& change : b.Data)
				newData.insert(change);
			return View{ std::move(newData) };
		}

		/// Check if two views are comparable.
		static bool areComparable(const View& a, const View& b) {
			return (a == b || a < b || a > b);
		}
	};
}}