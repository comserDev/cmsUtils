#include <cstddef>
#include <cstdint>

#include <cms/util/crypto/hmac_sha256.h>

#include "test.h"

namespace {

using cms::util::ByteView;
using cms::util::Status;
using cms::util::crypto::Sha256DigestSize;

bool bytesEqual(
    const std::uint8_t* lhs,
    const std::uint8_t* rhs,
    std::size_t size) noexcept {
    for (std::size_t i = 0; i < size; ++i) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

void checkHmac(
    ByteView key,
    ByteView data,
    const std::uint8_t (&expected)[Sha256DigestSize]) {
    std::uint8_t digest[Sha256DigestSize] = {};
    CMS_TEST_REQUIRE(
        cms::util::crypto::hmacSha256(key, data, digest) == Status::ok);
    CMS_TEST_CHECK(bytesEqual(digest, expected, Sha256DigestSize));
}

void testRfc4231Case1() {
    std::uint8_t key[20] = {};
    for (std::size_t i = 0; i < sizeof(key); ++i) {
        key[i] = 0x0BU;
    }
    const std::uint8_t data[] = "Hi There";
    const std::uint8_t expected[Sha256DigestSize] = {
        0xb0,0x34,0x4c,0x61,0xd8,0xdb,0x38,0x53,
        0x5c,0xa8,0xaf,0xce,0xaf,0x0b,0xf1,0x2b,
        0x88,0x1d,0xc2,0x00,0xc9,0x83,0x3d,0xa7,
        0x26,0xe9,0x37,0x6c,0x2e,0x32,0xcf,0xf7
    };
    checkHmac(ByteView(key), ByteView(data, sizeof(data) - 1), expected);
}

void testRfc4231Case2() {
    const std::uint8_t key[] = "Jefe";
    const std::uint8_t data[] = "what do ya want for nothing?";
    const std::uint8_t expected[Sha256DigestSize] = {
        0x5b,0xdc,0xc1,0x46,0xbf,0x60,0x75,0x4e,
        0x6a,0x04,0x24,0x26,0x08,0x95,0x75,0xc7,
        0x5a,0x00,0x3f,0x08,0x9d,0x27,0x39,0x83,
        0x9d,0xec,0x58,0xb9,0x64,0xec,0x38,0x43
    };
    checkHmac(
        ByteView(key, sizeof(key) - 1),
        ByteView(data, sizeof(data) - 1),
        expected);
}

void testRfc4231Case6() {
    std::uint8_t key[131] = {};
    for (std::size_t i = 0; i < sizeof(key); ++i) {
        key[i] = 0xAAU;
    }
    const std::uint8_t data[] =
        "Test Using Larger Than Block-Size Key - Hash Key First";
    const std::uint8_t expected[Sha256DigestSize] = {
        0x60,0xe4,0x31,0x59,0x1e,0xe0,0xb6,0x7f,
        0x0d,0x8a,0x26,0xaa,0xcb,0xf5,0xb7,0x7f,
        0x8e,0x0b,0xc6,0x21,0x37,0x28,0xc5,0x14,
        0x05,0x46,0x04,0x0f,0x0e,0xe3,0x7f,0x54
    };
    checkHmac(ByteView(key), ByteView(data, sizeof(data) - 1), expected);
}

void testRfc4231Case7() {
    std::uint8_t key[131] = {};
    for (std::size_t i = 0; i < sizeof(key); ++i) {
        key[i] = 0xAAU;
    }
    const std::uint8_t data[] =
        "This is a test using a larger than block-size key and a larger "
        "than block-size data. The key needs to be hashed before being "
        "used by the HMAC algorithm.";
    const std::uint8_t expected[Sha256DigestSize] = {
        0x9b,0x09,0xff,0xa7,0x1b,0x94,0x2f,0xcb,
        0x27,0x63,0x5f,0xbc,0xd5,0xb0,0xe9,0x44,
        0xbf,0xdc,0x63,0x64,0x4f,0x07,0x13,0x93,
        0x8a,0x7f,0x51,0x53,0x5c,0x3a,0x35,0xe2
    };
    checkHmac(ByteView(key), ByteView(data, sizeof(data) - 1), expected);
}

void testEmptyKeyAndData() {
    const std::uint8_t expected[Sha256DigestSize] = {
        0xb6,0x13,0x67,0x9a,0x08,0x14,0xd9,0xec,
        0x77,0x2f,0x95,0xd7,0x78,0xc3,0x5f,0xc5,
        0xff,0x16,0x97,0xc4,0x93,0x71,0x56,0x53,
        0xc6,0xc7,0x12,0x14,0x42,0x92,0xc5,0xad
    };
    checkHmac(ByteView(), ByteView(), expected);
}

void testEmptyAndBoundaryInputs() {
    const std::uint8_t keyByte[] = {0xA5U};
    const std::uint8_t dataByte[] = {0x5AU};
    const std::uint8_t emptyKeyExpected[Sha256DigestSize] = {
        0x54,0x3e,0x92,0x57,0xce,0xf5,0x46,0x97,
        0x08,0x75,0x9a,0xf4,0x0b,0xa2,0x2e,0x68,
        0xae,0xe9,0x5a,0xca,0xdb,0x5b,0x73,0x77,
        0x24,0x75,0x9d,0x9f,0x5b,0xb2,0x21,0x93
    };
    const std::uint8_t emptyDataExpected[Sha256DigestSize] = {
        0xd6,0xa7,0x69,0xc5,0x20,0xf1,0x8d,0x81,
        0xc3,0x1b,0xa6,0x89,0x60,0xfa,0xa9,0xef,
        0x84,0xd4,0x99,0x3c,0x0c,0x31,0x6c,0x36,
        0x16,0x31,0x74,0xce,0x62,0x26,0xe3,0x57
    };
    const std::uint8_t key64Expected[Sha256DigestSize] = {
        0xad,0xae,0x7f,0x2c,0x33,0x67,0xb2,0xe4,
        0x03,0x28,0x97,0xef,0x6d,0x85,0x23,0x8c,
        0x61,0x1f,0x42,0xf9,0x07,0x5a,0x19,0x4d,
        0x23,0x09,0x2f,0x6d,0x62,0x8a,0xbc,0xb2
    };
    const std::uint8_t key65Expected[Sha256DigestSize] = {
        0x2b,0xe5,0x28,0x95,0xe2,0xe9,0x0c,0x20,
        0x82,0x24,0x0f,0x48,0x65,0xac,0xf0,0xf1,
        0x59,0x9e,0x5c,0x5c,0x41,0x1a,0x38,0x8b,
        0x6a,0xe5,0x93,0x29,0x54,0x2d,0x24,0x1f
    };
    std::uint8_t key64[64] = {};
    std::uint8_t key65[65] = {};
    for (std::size_t i = 0; i < sizeof(key64); ++i) {
        key64[i] = static_cast<std::uint8_t>(i);
        key65[i] = key64[i];
    }
    key65[64] = 0xFFU;

    checkHmac(ByteView(), ByteView(dataByte), emptyKeyExpected);
    checkHmac(ByteView(keyByte), ByteView(), emptyDataExpected);
    checkHmac(ByteView(key64), ByteView(dataByte), key64Expected);
    checkHmac(ByteView(key65), ByteView(dataByte), key65Expected);
}

void testBinaryInputAndRepeatability() {
    const std::uint8_t key[] = {0x00U, 0xFFU, 0x80U, 0x01U};
    const std::uint8_t data[] = {0x80U, 0x00U, 0xFFU, 0x7FU, 0x00U};
    const std::uint8_t expected[Sha256DigestSize] = {
        0x4d,0xa6,0xdf,0x6c,0xc4,0x50,0x82,0xb2,
        0x5c,0xda,0x1a,0x20,0xa7,0xe2,0x36,0xe4,
        0x67,0xc8,0x77,0xd5,0x08,0x67,0xc4,0x3e,
        0x42,0x2a,0xa0,0x97,0x45,0x90,0x7c,0x25
    };
    std::uint8_t keyCopy[sizeof(key)] = {};
    std::uint8_t dataCopy[sizeof(data)] = {};
    for (std::size_t i = 0; i < sizeof(key); ++i) {
        keyCopy[i] = key[i];
    }
    for (std::size_t i = 0; i < sizeof(data); ++i) {
        dataCopy[i] = data[i];
    }

    std::uint8_t first[Sha256DigestSize] = {};
    std::uint8_t second[Sha256DigestSize] = {};
    CMS_TEST_REQUIRE(cms::util::crypto::hmacSha256(
        ByteView(key), ByteView(data), first) == Status::ok);
    CMS_TEST_REQUIRE(cms::util::crypto::hmacSha256(
        ByteView(key), ByteView(data), second) == Status::ok);
    CMS_TEST_CHECK(bytesEqual(first, expected, Sha256DigestSize));
    CMS_TEST_CHECK(bytesEqual(first, second, Sha256DigestSize));
    CMS_TEST_CHECK(bytesEqual(key, keyCopy, sizeof(key)));
    CMS_TEST_CHECK(bytesEqual(data, dataCopy, sizeof(data)));
}

void testNullViewsAreCanonicalEmpty() {
    const std::uint8_t* nullData = nullptr;
    const ByteView nullWithSize(nullData, 3);
    CMS_TEST_CHECK(nullWithSize.empty());

    std::uint8_t expected[Sha256DigestSize] = {};
    std::uint8_t actual[Sha256DigestSize] = {};
    CMS_TEST_REQUIRE(cms::util::crypto::hmacSha256(
        ByteView(), ByteView(), expected) == Status::ok);
    CMS_TEST_REQUIRE(cms::util::crypto::hmacSha256(
        nullWithSize, nullWithSize, actual) == Status::ok);
    CMS_TEST_CHECK(bytesEqual(actual, expected, Sha256DigestSize));
}

} // namespace

int main() {
    testRfc4231Case1();
    testRfc4231Case2();
    testRfc4231Case6();
    testRfc4231Case7();
    testEmptyKeyAndData();
    testEmptyAndBoundaryInputs();
    testBinaryInputAndRepeatability();
    testNullViewsAreCanonicalEmpty();
    return cms::test::finish();
}
