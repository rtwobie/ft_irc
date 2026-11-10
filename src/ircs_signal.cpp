#include "ircs_signal.h"

#include <signal.h>

#include "ircs_event_loop.h"
#include "ircs_logger.h"

namespace ircs {
using log::Logger;

extern bool loop_stop;

void signal_handler(int sig, siginfo_t *info, void *context)
{
    (void)sig;
    (void)info;
    (void)context;
    loop_stop = true;
    Logger(log::kInfo, "", "stopping server...");
}

}  // namespace ircs
