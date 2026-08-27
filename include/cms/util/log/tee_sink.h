#pragma once

#include <type_traits>
#include <utility>

#include <cms/util/status.h>
#include <cms/util/string_view.h>

namespace cms {
namespace util {
namespace log {

// 두 sink를 값으로 소유한다. Status 실패여도 둘 다 호출하지만 exception은
// catch하지 않으며 이미 성공한 output을 rollback하지 않는다.
template<class FirstSink, class SecondSink>
class TeeSink {
public:
    template<
        class First = FirstSink,
        class Second = SecondSink,
        typename std::enable_if<
            std::is_move_constructible<First>::value
                && std::is_move_constructible<Second>::value,
            int>::type = 0>
    TeeSink(FirstSink first, SecondSink second)
        noexcept(
            std::is_nothrow_move_constructible<First>::value
            && std::is_nothrow_move_constructible<Second>::value)
        : first_(std::move(first)), second_(std::move(second)) {}

    Status write(StringView data)
        noexcept(
            noexcept(std::declval<FirstSink&>().write(data))
            && noexcept(std::declval<SecondSink&>().write(data))) {
        const Status firstStatus = first_.write(data);
        const Status secondStatus = second_.write(data);
        return firstStatus != Status::ok ? firstStatus : secondStatus;
    }

private:
    FirstSink first_;
    SecondSink second_;
};

} // namespace log
} // namespace util
} // namespace cms
