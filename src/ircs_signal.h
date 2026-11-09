#ifndef IRCS_SIGNAL_H_
#define IRCS_SIGNAL_H_

#include <signal.h>

namespace ircs {

void signal_handler(int sig, siginfo_t *info, void *context);

}

#endif  // IRCS_SIGNAL_H_
