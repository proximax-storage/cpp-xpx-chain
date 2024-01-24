/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "View.h"

namespace catapult { namespace dbrb {

	bool View::isMember(const ProcessId& processId) const {
		return Data.find(processId) != Data.end();
	}

	size_t View::quorumSize() const {
		const size_t size = Data.size();
		return size - (size - 1) / 3;
	}

	size_t View::packedSize() const {
		return sizeof(uint32_t) + Data.size() * ProcessId_Size;
	}

	View& View::merge(View& other) {
		Data.merge(other.Data);
		return *this;
	}

	bool View::operator==(const View& other) const {
		return Data == other.Data;
	}

	bool View::operator!=(const View& other) const {
		return Data != other.Data;
	}

	bool View::operator<(const View& other) const {
		const auto& otherData = other.Data;
		const bool notEqual = (*this != other);
		return notEqual && std::includes(otherData.begin(), otherData.end(), Data.begin(), Data.end());
	}

	bool View::operator>(const View& other) const {
		const auto& otherData = other.Data;
		const bool notEqual = (*this != other);
		return notEqual && std::includes(Data.begin(), Data.end(), otherData.begin(), otherData.end());
	}

	bool View::operator<=(const View& other) const {
		return (*this < other) || (*this == other);
	}

	bool View::operator>=(const View& other) const {
		return (*this > other) || (*this == other);
	}
}}