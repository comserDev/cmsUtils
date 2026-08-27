#include <cstddef>
#include <cstdio>
#include <type_traits>
#include <utility>

#include <cms/util/log/async_logger.h>
#include <cms/util/platform/std_file_sink.h>
#include <cms/util/sync/null_mutex.h>

#include "test.h"

namespace {

constexpr const char* dataPath = "cms_std_file_sink_data.tmp";
constexpr const char* loggerPath = "cms_std_file_sink_logger.tmp";

struct FileCleanup {
    ~FileCleanup() noexcept {
        (void)std::remove(dataPath);
        (void)std::remove(loggerPath);
    }
};

struct FixedClock {
    cms::util::log::Timestamp nowMilliseconds() noexcept { return 42; }
};

std::size_t readBytes(const char* path, char* output, std::size_t capacity) {
    std::FILE* file = nullptr;
#if defined(_MSC_VER)
    (void)::fopen_s(&file, path, "rb");
#else
    file = std::fopen(path, "rb");
#endif
    if (file == nullptr) {
        return 0;
    }
    const std::size_t read = std::fread(output, 1, capacity, file);
    (void)std::fclose(file);
    return read;
}

void checkBytes(
    const char* actual,
    std::size_t actualSize,
    const char* expected,
    std::size_t expectedSize) {
    CMS_TEST_REQUIRE(actualSize == expectedSize);
    for (std::size_t index = 0; index < expectedSize; ++index) {
        CMS_TEST_CHECK(actual[index] == expected[index]);
    }
}

} // namespace

int main() {
    FileCleanup cleanup;
    (void)std::remove(dataPath);
    (void)std::remove(loggerPath);

    using cms::util::Status;
    using cms::util::platform::FileOpenMode;
    using cms::util::platform::StdFileSink;

    static_assert(!std::is_copy_constructible<StdFileSink>::value,
        "StdFileSink copy must be deleted");
    static_assert(std::is_nothrow_move_constructible<StdFileSink>::value,
        "StdFileSink move must be non-throwing");

    StdFileSink closed;
    CMS_TEST_CHECK(!closed.isOpen());
    CMS_TEST_CHECK(closed.write("x") == Status::invalid_argument);
    CMS_TEST_CHECK(closed.flush() == Status::invalid_argument);
    CMS_TEST_CHECK(closed.close() == Status::ok);
    CMS_TEST_CHECK(closed.open(nullptr) == Status::invalid_argument);
    CMS_TEST_CHECK(closed.open("") == Status::invalid_argument);
    CMS_TEST_CHECK(closed.open("cms_missing_parent__/file.bin")
        == Status::io_error);

    CMS_TEST_CHECK(closed.open(dataPath, FileOpenMode::truncate) == Status::ok);
    CMS_TEST_CHECK(closed.isOpen());
    CMS_TEST_CHECK(closed.open(dataPath) == Status::invalid_argument);
    CMS_TEST_CHECK(closed.write(cms::util::StringView()) == Status::ok);
    const char binary[] = {'A', '\0', 'B', '\n'};
    CMS_TEST_CHECK(closed.write(
        cms::util::StringView(binary, sizeof(binary))) == Status::ok);
    CMS_TEST_CHECK(closed.flush() == Status::ok);

    StdFileSink moved(std::move(closed));
    CMS_TEST_CHECK(!closed.isOpen());
    CMS_TEST_CHECK(moved.isOpen());
    CMS_TEST_CHECK(moved.close() == Status::ok);
    CMS_TEST_CHECK(!moved.isOpen());

    char readBuffer[64] = {};
    std::size_t read = readBytes(dataPath, readBuffer, sizeof(readBuffer));
    checkBytes(readBuffer, read, binary, sizeof(binary));

    StdFileSink append;
    CMS_TEST_CHECK(append.open(dataPath, FileOpenMode::append) == Status::ok);
    const char suffix[] = {'C', 'D'};
    CMS_TEST_CHECK(append.write(
        cms::util::StringView(suffix, sizeof(suffix))) == Status::ok);
    CMS_TEST_CHECK(append.close() == Status::ok);
    const char appended[] = {'A', '\0', 'B', '\n', 'C', 'D'};
    read = readBytes(dataPath, readBuffer, sizeof(readBuffer));
    checkBytes(readBuffer, read, appended, sizeof(appended));

    StdFileSink truncate;
    CMS_TEST_CHECK(truncate.open(dataPath, FileOpenMode::truncate) == Status::ok);
    CMS_TEST_CHECK(truncate.write("new") == Status::ok);
    CMS_TEST_CHECK(truncate.close() == Status::ok);
    read = readBytes(dataPath, readBuffer, sizeof(readBuffer));
    checkBytes(readBuffer, read, "new", 3);

    StdFileSink invalidMode;
    const FileOpenMode invalid = static_cast<FileOpenMode>(255);
    CMS_TEST_CHECK(invalidMode.open(dataPath, invalid)
        == Status::invalid_argument);
    CMS_TEST_CHECK(!invalidMode.isOpen());
    read = readBytes(dataPath, readBuffer, sizeof(readBuffer));
    checkBytes(readBuffer, read, "new", 3);

    {
        StdFileSink file;
        CMS_TEST_REQUIRE(file.open(loggerPath, FileOpenMode::truncate)
            == Status::ok);
        using Logger = cms::util::log::AsyncLogger<
            32,
            2,
            FixedClock,
            StdFileSink,
            cms::util::sync::NullMutex>;
        Logger logger{FixedClock(), std::move(file)};
        CMS_TEST_CHECK(!file.isOpen());
        CMS_TEST_CHECK(logger.log(cms::util::log::Level::info, "file")
            == Status::ok);
        CMS_TEST_CHECK(logger.drainOne() == Status::ok);
    }
    read = readBytes(loggerPath, readBuffer, sizeof(readBuffer));
    checkBytes(readBuffer, read, "[42] [INFO] file\n", 17);

    std::printf("sizeof(cms::util::Status)=%zu\n", sizeof(Status));
    std::printf("sizeof(cms::util::platform::StdFileSink)=%zu\n",
        sizeof(StdFileSink));
    std::printf("alignof(cms::util::platform::StdFileSink)=%zu\n",
        alignof(StdFileSink));

    return cms::test::finish();
}
