#ifndef IRCS_CLIENT_IO_H_
#define IRCS_CLIENT_IO_H_

#include <string>

#include "ircs_client.h"

namespace ircs {

std::string ServerPrefix(std::string &server_name);
std::string UserPrefix(Client *client);
void QueueRawReply(Client *client, const std::string &message);
void QueueNumericReply(Client *client, int numeric, const std::string &message);
void QueueCommonChannelsMessage(Client *client, const std::string &message, bool include_self);
void QueueChannelMessage(Channel *channel, const std::string &message,
                         const std::map<std::string, Client *> &members, Client *except);

}  // namespace ircs

#endif  // IRCS_CLIENT_IO_H_
