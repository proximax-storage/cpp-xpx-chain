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

#include "catapult/utils/ConfigurationValueParsers.h"
#include "catapult/utils/BlockSpan.h"
#include "catapult/utils/FileSize.h"
#include "catapult/utils/TimeSpan.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace utils {

#define TEST_CLASS ConfigurationValueParsersTests

	namespace {
		template<typename T>
		bool TryParseValueT(const std::string& input, T& parsedValue) {
			return TryParseValue(input, parsedValue);
		}

		template<typename T>
		void AssertSuccessfulParse(const std::string& input, const T& expectedParsedValue) {
			test::AssertParse(input, expectedParsedValue, TryParseValueT<T>);
		}

		template<typename T>
		void AssertFailedParse(const std::string& input, const T& initialValue) {
			test::AssertFailedParse(input, initialValue, TryParseValueT<T>);
		}
	}

	// region discrete enums

	namespace {
		template<typename T>
		void AssertEnumParseFailure(const std::string& seed, const T& initialValue) {
			test::AssertEnumParseFailure(seed, initialValue, TryParseValueT<T>);
		}

		const std::array<std::pair<const char*, int>, 4> String_To_Square_Mapping{{
			{ std::make_pair("one", 1) },
			{ std::make_pair("two", 4) },
			{ std::make_pair("three", 9) },
			{ std::make_pair("four", 16) },
		}};
	}

	TEST(TEST_CLASS, CanParseValidEnumValue) {
		// Arrange:
		auto assertSuccessfulParse = [](const auto& input, const auto& expectedParsedValue) {
			test::AssertParse(input, expectedParsedValue, [](const auto& str, auto& parsedValue) {
				return TryParseEnumValue(String_To_Square_Mapping, str, parsedValue);
			});
		};

		// Assert:
		assertSuccessfulParse("one", 1);
		assertSuccessfulParse("two", 4);
		assertSuccessfulParse("three", 9);
		assertSuccessfulParse("four", 16);
	}

	TEST(TEST_CLASS, CannotParseInvalidEnumValue) {
		// Assert:
		test::AssertEnumParseFailure("two", 7, [](const auto& str, auto& parsedValue) {
			return TryParseEnumValue(String_To_Square_Mapping, str, parsedValue);
		});
	}

	// endregion

	// region bitwise enums

	TEST(TEST_CLASS, CanParseEmptyStringAsBitwiseEnumValue) {
		// Act + Assert:
		test::AssertParse("", 0, [](const auto& str, auto& parsedValue) {
			return TryParseBitwiseEnumValue(String_To_Square_Mapping, str, parsedValue);
		});
	}

	TEST(TEST_CLASS, CanParseSingleValueAsBitwiseEnumValue) {
		// Act + Assert:
		test::AssertParse("three", 9, [](const auto& str, auto& parsedValue) {
			return TryParseBitwiseEnumValue(String_To_Square_Mapping, str, parsedValue);
		});
	}

	TEST(TEST_CLASS, CanParseMultipleValuesAsBitwiseEnumValue) {
		// Act + Assert:
		test::AssertParse("two,four", 20, [](const auto& str, auto& parsedValue) {
			return TryParseBitwiseEnumValue(String_To_Square_Mapping, str, parsedValue);
		});
	}

	TEST(TEST_CLASS, CannotParseMalformedSetAsBitwiseEnumValue) {
		// Act + Assert:
		test::AssertFailedParse("two,,four", 0, [](const auto& str, auto& parsedValue) {
			return TryParseBitwiseEnumValue(String_To_Square_Mapping, str, parsedValue);
		});
	}

	TEST(TEST_CLASS, CannotParseSetWithUnknownValueAsBitwiseEnumValue) {
		// Act + Assert:
		test::AssertFailedParse("two,five,four", 0, [](const auto& str, auto& parsedValue) {
			return TryParseBitwiseEnumValue(String_To_Square_Mapping, str, parsedValue);
		});
	}

	// endregion

	// region log related enums

	TEST(TEST_CLASS, CanParseValidLogLevel) {
		// Assert:
		using T = LogLevel;
		AssertSuccessfulParse("Trace", T::Trace);
		AssertSuccessfulParse("Debug", T::Debug);
		AssertSuccessfulParse("Info", T::Info);
		AssertSuccessfulParse("Warning", T::Warning);
		AssertSuccessfulParse("Error", T::Error);
		AssertSuccessfulParse("Fatal", T::Fatal);
		AssertSuccessfulParse("Min", T::Min);
		AssertSuccessfulParse("Max", T::Max);
	}

	TEST(TEST_CLASS, CannotParseInvalidLogLevel) {
		// Assert:
		AssertEnumParseFailure("Warning", LogLevel::Info);
	}

	TEST(TEST_CLASS, CanParseValidLogSinkType) {
		// Assert:
		using T = LogSinkType;
		AssertSuccessfulParse("Sync", T::Sync);
		AssertSuccessfulParse("Async", T::Async);
	}

	TEST(TEST_CLASS, CannotParseInvalidLogSinkType) {
		// Assert:
		AssertEnumParseFailure("Sync", LogSinkType::Async);
	}

	TEST(TEST_CLASS, CanParseValidLogColorMode) {
		// Assert:
		using T = LogColorMode;
		AssertSuccessfulParse("Ansi", T::Ansi);
		AssertSuccessfulParse("AnsiBold", T::AnsiBold);
		AssertSuccessfulParse("None", T::None);
	}

	TEST(TEST_CLASS, CannotParseInvalidLogColorMode) {
		// Assert:
		AssertEnumParseFailure("Ansi", LogColorMode::None);
	}

	// endregion

	// region bool

	TEST(TEST_CLASS, CanParseValidBoolean) {
		// Assert:
		AssertSuccessfulParse("true", true);
		AssertSuccessfulParse("false", false);
	}

	TEST(TEST_CLASS, CannotParseInvalidBoolean) {
		// Assert:
		AssertEnumParseFailure("false", true);
	}

	// endregion

	// region int - utils

	namespace {
		template<typename TNumeric>
		std::string ToUnsignedIntHexString(TNumeric value) {
			return "0x" + test::ToString(utils::HexFormat(value));
		}

		template<typename TNumeric, typename TFactory>
		void AssertUnsignedIntDecimalParseSuccess(TNumeric expectedMaxValue, const std::string& postfix, TFactory factory) {
			using NumericLimits = std::numeric_limits<TNumeric>;
			AssertSuccessfulParse(std::to_string(NumericLimits::min()) + postfix, factory(static_cast<TNumeric>(0))); // min
			AssertSuccessfulParse("1" + postfix, factory(1)); // other values
			AssertSuccessfulParse("1234" + postfix, factory(1234));
			AssertSuccessfulParse("8692" + postfix, factory(8692));
			AssertSuccessfulParse("8'692" + postfix, factory(8692)); // with separators
			AssertSuccessfulParse("8'6'9'2" + postfix, factory(8692));
			AssertSuccessfulParse(std::to_string(NumericLimits::max()) + postfix, factory(expectedMaxValue)); // max
		}

		template<typename TNumeric, typename TFactory>
		void AssertUnsignedIntHexParseSuccess(TNumeric expectedMaxValue, const std::string& postfix, TFactory factory) {
			using NumericLimits = std::numeric_limits<TNumeric>;
			AssertSuccessfulParse(ToUnsignedIntHexString(NumericLimits::min()) + postfix, factory(static_cast<TNumeric>(0))); // min
			AssertSuccessfulParse("0x1" + postfix, factory(0x0001)); // other values
			AssertSuccessfulParse("0x1234" + postfix, factory(0x1234));
			AssertSuccessfulParse("0x8F9A" + postfix, factory(0x8F9A));
			AssertSuccessfulParse("0x8'F9A" + postfix, factory(0x8F9A)); // with separators
			AssertSuccessfulParse("0x8'F'9'A" + postfix, factory(0x8F9A));
			AssertSuccessfulParse(ToUnsignedIntHexString(NumericLimits::max()) + postfix, factory(expectedMaxValue)); // max
		}

		template<typename T, typename TNumeric>
		void AssertUnsignedIntDecimalParseSuccess(TNumeric expectedMaxValue) {
			AssertUnsignedIntDecimalParseSuccess<TNumeric>(expectedMaxValue, "", [](auto raw) { return T(static_cast<TNumeric>(raw)); });
		}

		template<typename T>
		void AssertUnsignedIntDecimalParseSuccess(T expectedMaxValue) {
			AssertUnsignedIntDecimalParseSuccess<T, T>(expectedMaxValue);
		}

		template<typename T>
		void AssertUnsignedIntDecimalParseFailureBasic(const T& initialValue, const std::string& prefix, const std::string& postfix) {
			AssertFailedParse("", initialValue); // empty
			AssertFailedParse(prefix, initialValue); // prefix only
			AssertFailedParse(postfix, initialValue); // postfix only
			AssertFailedParse(prefix + postfix, initialValue); // prefix and postfix only

			AssertFailedParse(prefix + "'1234" + postfix, initialValue); // leading separator
			AssertFailedParse(prefix + "1234'" + postfix, initialValue); // trailing separator
			AssertFailedParse(prefix + "1''234" + postfix, initialValue); // consecutive separators
			AssertFailedParse(" " + prefix + "1234" + postfix, initialValue); // leading space
			AssertFailedParse(prefix + "1234" + postfix + " ", initialValue); // trailing space
			AssertFailedParse("-" + prefix + "1" + postfix, initialValue); // too small

			AssertFailedParse(prefix + "@#$" + postfix, initialValue); // non numeric
			AssertFailedParse(prefix + "lmn" + postfix, initialValue);
			AssertFailedParse(prefix + "10.25" + postfix, initialValue); // non integral
			for (auto invalidDigit : { 'Z', 'a', '$', '=' })
				AssertFailedParse(prefix + "10" + invalidDigit + "25" + postfix, initialValue); // invalid digit
		}

		template<typename T, typename TNumeric>
		void AssertUnsignedIntDecimalParseFailure(const T& initialValue, const std::string& postfix) {
			// basic
			AssertUnsignedIntDecimalParseFailureBasic(initialValue, "", postfix);

			// too large
			using NumericLimits = std::numeric_limits<TNumeric>;
			auto maxString = std::to_string(NumericLimits::max());
			AssertFailedParse(maxString + "0" + postfix, initialValue); // 10x too large
			++maxString.back();
			AssertFailedParse(maxString + postfix, initialValue); // 1 too large

			// other invalid char
			AssertFailedParse("0x10A25" + postfix, initialValue); // non integral
		}

		template<typename T, typename TNumeric>
		void AssertUnsignedIntHexParseFailure(const T& initialValue, const std::string& postfix) {
			// basic
			AssertUnsignedIntDecimalParseFailureBasic(initialValue, "0x", postfix);

			// invalid prefix
			AssertFailedParse("0", initialValue); // too short
			AssertFailedParse("1234", initialValue); // missing prefix
			AssertFailedParse("01234", initialValue); // invalid prefix
			AssertFailedParse("0" "X1234", initialValue);
			AssertFailedParse("2x1234", initialValue);

			// too large
			using NumericLimits = std::numeric_limits<TNumeric>;
			auto maxString = ToUnsignedIntHexString(NumericLimits::max());
			AssertFailedParse("0x" + maxString + "0" + postfix, initialValue); // 16x too large
			maxString = "1" + std::string(maxString.size(), '0');
			AssertFailedParse("0x" + maxString + postfix, initialValue); // 1 too large
		}

		template<typename T, typename TNumeric>
		void AssertUnsignedIntDecimalParseFailure() {
			AssertUnsignedIntDecimalParseFailure<T, TNumeric>(T(177), "");
		}

		template<typename T>
		void AssertUnsignedIntDecimalParseFailure() {
			AssertUnsignedIntDecimalParseFailure<T, T>();
		}
	}

	// endregion

	// region int

	TEST(TEST_CLASS, CanParseValidUInt8) {
		// Assert:
		AssertUnsignedIntDecimalParseSuccess(static_cast<uint8_t>(0xFF));
	}

	TEST(TEST_CLASS, CannotParseInvalidUInt8) {
		// Assert:
		AssertUnsignedIntDecimalParseFailure<uint8_t>();
	}

	TEST(TEST_CLASS, CanParseValidUInt16) {
		// Assert:
		AssertUnsignedIntDecimalParseSuccess(static_cast<uint16_t>(0xFFFF));
	}

	TEST(TEST_CLASS, CannotParseInvalidUInt16) {
		// Assert:
		AssertUnsignedIntDecimalParseFailure<uint16_t>();
	}

	TEST(TEST_CLASS, CanParseValidUInt32) {
		// Assert:
		AssertUnsignedIntDecimalParseSuccess(static_cast<uint32_t>(0xFFFF'FFFF));
	}

	TEST(TEST_CLASS, CannotParseInvalidUInt32) {
		// Assert:
		AssertUnsignedIntDecimalParseFailure<uint32_t>();
	}

	TEST(TEST_CLASS, CanParseValidUInt64) {
		// Assert:
		AssertUnsignedIntDecimalParseSuccess(static_cast<uint64_t>(0xFFFF'FFFF'FFFF'FFFF));
	}

	TEST(TEST_CLASS, CannotParseInvalidUInt64) {
		// Assert:
		AssertUnsignedIntDecimalParseFailure<uint64_t>();
	}

	// endregion

	// region custom types

	TEST(TEST_CLASS, CanParseValidAmount) {
		// Assert:
		AssertUnsignedIntDecimalParseSuccess<Amount, Amount::ValueType>(0xFFFF'FFFF'FFFF'FFFF);
	}

	TEST(TEST_CLASS, CannotParseInvalidAmount) {
		// Assert:
		AssertUnsignedIntDecimalParseFailure<Amount, Amount::ValueType>();
	}

	TEST(TEST_CLASS, CanParseValidBlockFeeMultiplier) {
		// Assert:
		AssertUnsignedIntDecimalParseSuccess<BlockFeeMultiplier, BlockFeeMultiplier::ValueType>(0xFFFF'FFFF);
	}

	TEST(TEST_CLASS, CannotParseInvalidBlockFeeMultiplier) {
		// Assert:
		AssertUnsignedIntDecimalParseFailure<BlockFeeMultiplier, BlockFeeMultiplier::ValueType>();
	}

	TEST(TEST_CLASS, CanParseValidHeight) {
		// Assert:
		AssertUnsignedIntDecimalParseSuccess<Height, Height::ValueType>(0xFFFF'FFFF'FFFF'FFFF);
	}

	TEST(TEST_CLASS, CannotParseInvalidHeight) {
		// Assert:
		AssertUnsignedIntDecimalParseFailure<Height, Height::ValueType>();
	}

	TEST(TEST_CLASS, CanParseValidImportance) {
		// Assert:
		AssertUnsignedIntDecimalParseSuccess<Importance, Importance::ValueType>(0xFFFF'FFFF'FFFF'FFFF);
	}

	TEST(TEST_CLASS, CannotParseInvalidImportance) {
		// Assert:
		AssertUnsignedIntDecimalParseFailure<Importance, Importance::ValueType>();
	}

	TEST(TEST_CLASS, CanParseValidMosaicId) {
		// Assert:
		AssertUnsignedIntHexParseSuccess<MosaicId::ValueType>(0xFFFF'FFFF'FFFF'FFFF, "", [](auto value) {
			return MosaicId(static_cast<MosaicId::ValueType>(value));
		});
	}

	TEST(TEST_CLASS, CannotParseInvalidMosaicId) {
		// Assert:
		AssertUnsignedIntHexParseFailure<MosaicId, MosaicId::ValueType>(MosaicId(177), "");
	}

	TEST(TEST_CLASS, CanParseValidTimeSpan) {
		// Assert:
		auto maxValue = 0xFFFF'FFFF'FFFF'FFFF;
		AssertUnsignedIntDecimalParseSuccess<uint64_t>(maxValue, "ms", TimeSpan::FromMilliseconds);
		AssertUnsignedIntDecimalParseSuccess<uint64_t>(maxValue, "s", TimeSpan::FromSeconds);
		AssertUnsignedIntDecimalParseSuccess<uint64_t>(maxValue, "m", TimeSpan::FromMinutes);
		AssertUnsignedIntDecimalParseSuccess<uint64_t>(maxValue, "h", TimeSpan::FromHours);
	}

	TEST(TEST_CLASS, CannotParseInvalidTimeSpan) {
		// Assert:
		auto initialValue = TimeSpan::FromMinutes(8888);
		AssertUnsignedIntDecimalParseFailure<TimeSpan, uint64_t>(initialValue, "m");

		AssertFailedParse("1234", initialValue); // no specifier
		AssertFailedParse("12s34", initialValue); // non-terminating specifier
		AssertFailedParse("1234S", initialValue); // wrong case specifier
		AssertFailedParse("1234d", initialValue); // unknown specifier
	}

	TEST(TEST_CLASS, CanParseValidBlockSpan) {
		// Assert:
		auto maxValue = 0xFFFF'FFFF'FFFF'FFFF;
		AssertUnsignedIntDecimalParseSuccess<uint64_t>(maxValue, "h", BlockSpan::FromHours);
		AssertUnsignedIntDecimalParseSuccess<uint64_t>(maxValue, "d", BlockSpan::FromDays);
	}

	TEST(TEST_CLASS, CannotParseInvalidBlockSpan) {
		// Assert:
		auto initialValue = BlockSpan::FromHours(8888);
		AssertUnsignedIntDecimalParseFailure<BlockSpan, uint64_t>(initialValue, "h");

		AssertFailedParse("1234", initialValue); // no specifier
		AssertFailedParse("12h34", initialValue); // non-terminating specifier
		AssertFailedParse("1234H", initialValue); // wrong case specifier
		AssertFailedParse("1234s", initialValue); // unknown specifier
	}

	TEST(TEST_CLASS, CanParseValidFileSize) {
		// Assert:
		auto maxValue = 0xFFFF'FFFF'FFFF'FFFF;
		AssertUnsignedIntDecimalParseSuccess<uint64_t>(maxValue, "B", FileSize::FromBytes);
		AssertUnsignedIntDecimalParseSuccess<uint64_t>(maxValue, "KB", FileSize::FromKilobytes);
		AssertUnsignedIntDecimalParseSuccess<uint64_t>(maxValue, "MB", FileSize::FromMegabytes);
		AssertUnsignedIntDecimalParseSuccess<uint64_t>(maxValue, "GB", FileSize::FromGigabytes);
		AssertUnsignedIntDecimalParseSuccess<uint64_t>(maxValue, "TB", FileSize::FromTerabytes);
	}

	TEST(TEST_CLASS, CannotParseInvalidFileSize) {
		// Assert:
		auto initialValue = FileSize::FromKilobytes(8888);
		AssertUnsignedIntDecimalParseFailure<FileSize, uint64_t>(initialValue, "KB");

		AssertFailedParse("1234", initialValue); // no specifier
		AssertFailedParse("12KB34", initialValue); // non-terminating specifier
		AssertFailedParse("1234Kb", initialValue); // wrong case specifier
	}

	// endregion

	// region byte array

	namespace {
		template<typename TByteArray>
		void AssertCanParseValidByteArray() {
			// Assert:
			AssertSuccessfulParse("031729D10DB52ECF0AD3684558DB31895DDFA5CD7F4143AF6E822E114E16E31C", TByteArray{{
				0x03, 0x17, 0x29, 0xD1, 0x0D, 0xB5, 0x2E, 0xCF, 0x0A, 0xD3, 0x68, 0x45, 0x58, 0xDB, 0x31, 0x89,
				0x5D, 0xDF, 0xA5, 0xCD, 0x7F, 0x41, 0x43, 0xAF, 0x6E, 0x82, 0x2E, 0x11, 0x4E, 0x16, 0xE3, 0x1C
			}});
			AssertSuccessfulParse("AB1729D10DB52ECF0AD3684558DB31895DDFA5CD7F4143AF6E822E114E16E300", TByteArray{{
				0xAB, 0x17, 0x29, 0xD1, 0x0D, 0xB5, 0x2E, 0xCF, 0x0A, 0xD3, 0x68, 0x45, 0x58, 0xDB, 0x31, 0x89,
				0x5D, 0xDF, 0xA5, 0xCD, 0x7F, 0x41, 0x43, 0xAF, 0x6E, 0x82, 0x2E, 0x11, 0x4E, 0x16, 0xE3, 0x00
			}});
		}

		template<typename TByteArray>
		void AssertCannotParseInvalidByteArray() {
			// Arrange:
			TByteArray initialValue{ { 0x25 } };

			// Assert
			AssertFailedParse("031729D10DB52ECF0AD3684558DB3189@DDFA5CD7F4143AF6E822E114E16E31C", initialValue); // invalid char
			AssertFailedParse("31729D10DB52ECF0AD3684558DB31895DDFA5CD7F4143AF6E822E114E16E31C", initialValue); // too short (odd)
			AssertFailedParse("1729D10DB52ECF0AD3684558DB31895DDFA5CD7F4143AF6E822E114E16E31C", initialValue); // too short (even)
			AssertFailedParse("031729D10DB52ECF0AD3684558DB31895DDFA5CD7F4143AF6E822E114E16E31CA", initialValue); // too long (odd)
			AssertFailedParse("031729D10DB52ECF0AD3684558DB31895DDFA5CD7F4143AF6E822E114E16E31CAB", initialValue); // too long (even)
		}
	}

	TEST(TEST_CLASS, CanParseValidKey) {
		// Assert:
		AssertCanParseValidByteArray<Key>();
	}

	TEST(TEST_CLASS, CannotParseInvalidKey) {
		// Assert:
		AssertCannotParseInvalidByteArray<Key>();
	}

	TEST(TEST_CLASS, CanParseValidHash256) {
		// Assert:
		AssertCanParseValidByteArray<Hash256>();
	}

	TEST(TEST_CLASS, CannotParseInvalidHash256) {
		// Assert:
		AssertCannotParseInvalidByteArray<Hash256>();
	}

	TEST(TEST_CLASS, CanParseValidGenerationHash) {
		// Assert:
		AssertCanParseValidByteArray<GenerationHash>();
	}

	TEST(TEST_CLASS, CannotParseInvalidGenerationHash) {
		// Assert:
		AssertCannotParseInvalidByteArray<GenerationHash>();
	}

	// endregion

	// region string

	TEST(TEST_CLASS, CanParseString) {
		// Assert:
		AssertSuccessfulParse("Foo BAR", std::string("Foo BAR"));
		AssertSuccessfulParse("Ac$D*a98p124!", std::string("Ac$D*a98p124!"));
	}

	// endregion

	// region set

	TEST(TEST_CLASS, CanParseValidUnorderedSetOfString) {
		// Arrange:
		using Container = std::unordered_set<std::string>;

		// Assert:
		AssertSuccessfulParse("", Container()); // no values
		AssertSuccessfulParse("alpha", Container{ "alpha" });
		AssertSuccessfulParse("alpha,bEta,gammA", Container{ "alpha", "bEta", "gammA" });
		AssertSuccessfulParse("\talpha\t,  bEta  "", gammA,zeta ", Container{ "alpha", "bEta", "gammA", "zeta" });
		AssertSuccessfulParse("Foo BAR,Ac$D*a98p124!", Container{ "Foo BAR", "Ac$D*a98p124!" });
	}

	TEST(TEST_CLASS, CannotParseInvalidUnorderedSetOfString) {
		// Arrange:
		std::unordered_set<std::string> initialValue{ "default", "values" };

		// Assert
		AssertFailedParse(",", initialValue); // no values
		AssertFailedParse("alpha,,gammA", initialValue); // empty value (middle)
		AssertFailedParse("alpha,gammA,", initialValue); // empty value (last)
		AssertFailedParse(",alpha,gammA", initialValue); // empty value (first)
		AssertFailedParse("alpha, \t \t,gammA", initialValue); // whitespace value
		AssertFailedParse("alpha,beta,alpha", initialValue); // duplicate values
	}

	// endregion
}}
