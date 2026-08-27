#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include <cms/util/platform/arduino_udp_sink.h>

namespace {

struct FakeAddress {};

struct FakeUdp {
    int beginPacket(FakeAddress, std::uint16_t);
    std::size_t write(const std::uint8_t*, std::size_t);
    int endPacket();
};

using Sink = cms::util::platform::ArduinoUdpSink<FakeUdp, FakeAddress>;

static_assert(std::is_constructible<
    Sink,
    FakeUdp&,
    FakeAddress,
    std::uint16_t>::value,
    "ArduinoUdpSink must accept a non-owning UDP reference");
static_assert(std::is_same<
    decltype(std::declval<Sink&>().write(cms::util::StringView())),
    cms::util::Status>::value,
    "ArduinoUdpSink write must return Status");

} // namespace
