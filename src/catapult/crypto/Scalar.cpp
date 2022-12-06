/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Scalar.h"
#include <cstdio>
extern "C" {
#include <external/ref10/sc.h>
}

namespace catapult::crypto {
	Scalar::Scalar(const std::array<uint8_t, Scalar_Size * 2>& arr) {
		auto temp = arr;
		sc_reduce(temp.data());
		std::copy(temp.begin(), temp.begin() + Scalar_Size, m_array.begin());
	}

	Scalar::Scalar(const std::array<uint8_t, Scalar_Size>& arr) {
		std::array<uint8_t, Scalar_Size * 2> temp{};
		std::copy(arr.begin(), arr.end(), temp.begin());
		sc_reduce(temp.data());
		std::copy(temp.begin(), temp.begin() + Scalar_Size, m_array.begin());
	}

	Scalar::Scalar() : ByteArray() {}

	Scalar::~Scalar() {
		m_array.fill(0);
	}

	Scalar Scalar::getLMinusOne() {
		Scalar ret;
		ret.m_array = Scalar::L_MINUS_ONE;
		return ret;
	}

	Scalar Scalar::operator+(const Scalar& a) const {
		Scalar temp;
		Scalar one;
		one[0] = 1;
		sc_muladd(temp.data(), this->m_array.data(), one.m_array.data(), a.m_array.data());

		return temp;
	}

	Scalar Scalar::operator*(const Scalar& a) const {
		Scalar temp;
		Scalar zero;
		sc_muladd(temp.data(), this->m_array.data(), a.m_array.data(), zero.m_array.data());

		return temp;
	}

	Scalar Scalar::operator-(const Scalar& a) const {
		Scalar temp;
		Scalar iMinusOne = Scalar::getLMinusOne();
		sc_muladd(temp.data(), a.m_array.data(), iMinusOne.m_array.data(), m_array.data());

		return temp;
	}

	Scalar Scalar::operator-() const {
		Scalar temp;
		temp.m_array = this->m_array;
		temp -= *this;
		temp -= *this;
		return temp;
	}

	Scalar& Scalar::operator+=(const Scalar& a) {
		*this = *this + a;
		return *this;
	}

	Scalar& Scalar::operator-=(const Scalar& a) {
		*this = *this - a;
		return *this;
	}

	Scalar& Scalar::operator*=(const Scalar& a) {
		*this = *this * a;
		return *this;
	}

	Scalar Scalar::addProduct(const Scalar& r, const Scalar& h) const {
		Scalar temp;
		sc_muladd(temp.data(), r.m_array.data(), h.m_array.data(), m_array.data());
		return temp;
	}
} // namespace catapult::crypto