#include "core/Signals.hpp"

#include <csignal>

namespace
{
    volatile std::sig_atomic_t g_stopRequested = 0;

    extern "C" void onStopSignal(int)
    {
        g_stopRequested = 1;
    }
}

namespace Signals
{
    void install()
    {
        signal(SIGPIPE, SIG_IGN);
        signal(SIGINT, onStopSignal);
        signal(SIGTERM, onStopSignal);
    }

    bool stopRequested()
    {
        return g_stopRequested != 0;
    }
}
