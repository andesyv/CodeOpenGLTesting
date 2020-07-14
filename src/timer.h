#include <chrono>

template <typename ct = std::chrono::steady_clock>
class basic_timer
{
private:
    typename ct::time_point tp;

public:
    basic_timer()
        : tp{ct::now()}
    {}

    void reset() {
        tp = ct::now();
    }

    template <typename T = std::chrono::nanoseconds>
    auto elapsed() const
    {
        auto duration = std::chrono::duration_cast<T>(ct::now() - tp);
        return duration.count();
    }

    template <typename T = std::chrono::nanoseconds>
    auto elapsedReset()
    {
        auto ret = elapsed<T>();
        reset();
        return ret;
    }
};

class Timer : public basic_timer<std::chrono::steady_clock>
{};