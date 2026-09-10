#include <cstddef>
#include <cstdint>

#include <cms/util/crypto/constant_time.h>

#include "test.h"

namespace {

using cms::util::ByteView;
using cms::util::crypto::constantTimeEqual;

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

void testEmptyAndSingleByte() {
    const std::uint8_t zero[] = {0x00U};
    const std::uint8_t one[] = {0x01U};

    CMS_TEST_CHECK(constantTimeEqual(ByteView(), ByteView()));
    CMS_TEST_CHECK(!constantTimeEqual(ByteView(), ByteView(zero)));
    CMS_TEST_CHECK(!constantTimeEqual(ByteView(zero), ByteView()));
    CMS_TEST_CHECK(constantTimeEqual(ByteView(zero), ByteView(zero)));
    CMS_TEST_CHECK(!constantTimeEqual(ByteView(zero), ByteView(one)));
}

void testDigestSizedInputs() {
    std::uint8_t expected[32] = {};
    for (std::size_t i = 0; i < sizeof(expected); ++i) {
        expected[i] = static_cast<std::uint8_t>(i * 7U + 3U);
    }

    std::uint8_t same[32] = {};
    std::uint8_t firstMismatch[32] = {};
    std::uint8_t middleMismatch[32] = {};
    std::uint8_t lastMismatch[32] = {};
    for (std::size_t i = 0; i < sizeof(expected); ++i) {
        same[i] = expected[i];
        firstMismatch[i] = expected[i];
        middleMismatch[i] = expected[i];
        lastMismatch[i] = expected[i];
    }
    firstMismatch[0] ^= 0x01U;
    middleMismatch[sizeof(middleMismatch) / 2] ^= 0x80U;
    lastMismatch[sizeof(lastMismatch) - 1] ^= 0xFFU;

    CMS_TEST_CHECK(constantTimeEqual(ByteView(expected), ByteView(same)));
    CMS_TEST_CHECK(!constantTimeEqual(
        ByteView(expected), ByteView(firstMismatch)));
    CMS_TEST_CHECK(!constantTimeEqual(
        ByteView(expected), ByteView(middleMismatch)));
    CMS_TEST_CHECK(!constantTimeEqual(
        ByteView(expected), ByteView(lastMismatch)));
    CMS_TEST_CHECK(!constantTimeEqual(
        ByteView(expected, sizeof(expected) - 1), ByteView(same)));
}

void testBinaryBytes() {
    const std::uint8_t lhs[] = {0x00U, 0x80U, 0xFFU, 0x00U};
    const std::uint8_t equal[] = {0x00U, 0x80U, 0xFFU, 0x00U};
    const std::uint8_t different[] = {0x00U, 0x80U, 0xFEU, 0x00U};

    CMS_TEST_CHECK(constantTimeEqual(ByteView(lhs), ByteView(equal)));
    CMS_TEST_CHECK(!constantTimeEqual(ByteView(lhs), ByteView(different)));
}

void testRepeatabilityAndInputPreservation() {
    const std::uint8_t lhs[] = {0x10U, 0x00U, 0x80U, 0xFFU, 0x20U};
    const std::uint8_t rhs[] = {0x10U, 0x00U, 0x80U, 0xFEU, 0x20U};
    const std::uint8_t lhsCopy[] = {0x10U, 0x00U, 0x80U, 0xFFU, 0x20U};
    const std::uint8_t rhsCopy[] = {0x10U, 0x00U, 0x80U, 0xFEU, 0x20U};

    for (std::size_t i = 0; i < 16; ++i) {
        CMS_TEST_CHECK(!constantTimeEqual(ByteView(lhs), ByteView(rhs)));
    }
    CMS_TEST_CHECK(bytesEqual(lhs, lhsCopy, sizeof(lhs)));
    CMS_TEST_CHECK(bytesEqual(rhs, rhsCopy, sizeof(rhs)));
}

} // namespace

int main() {
    testEmptyAndSingleByte();
    testDigestSizedInputs();
    testBinaryBytes();
    testRepeatabilityAndInputPreservation();
    return cms::test::finish();
}
