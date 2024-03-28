/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/types.h"

namespace catapult::crypto {

	constexpr size_t Scalar_Size = 32;

	struct Scalar_Tag {
		static constexpr auto Byte_Size = 32;
	};

	class Scalar : public utils::ByteArray<Scalar_Size, Scalar_Tag> {
	private:
		static constexpr std::array<uint8_t, Scalar_Size> L_MINUS_ONE = { 236, 211, 245, 92,  26,  99,	18,	 88,
																		  214, 156, 247, 162, 222, 249, 222, 20,
																		  0,   0,	0,	 0,	  0,   0,	0,	 0,
																		  0,   0,	0,	 0,	  0,   0,	0,	 16 };

	public:
		Scalar();
		~Scalar();
		Scalar(const std::array<uint8_t, Scalar_Size * 2>& arr);
		Scalar(const std::array<uint8_t, Scalar_Size>& arr);
		static Scalar getLMinusOne();
		Scalar operator+(const Scalar& a) const;
		Scalar operator*(const Scalar& a) const;
		Scalar operator-(const Scalar& a) const;
		Scalar operator-() const;
		Scalar& operator+=(const Scalar& a);
		Scalar& operator*=(const Scalar& a);
		Scalar& operator-=(const Scalar& a);
		Scalar addProduct(const Scalar& r, const Scalar& h) const;
	};

} // namespace catapult::crypto