#include <iomanip>
#include <sstream>

#include "ircs_client.h"
#include "ircs_message.h"

namespace ircs {

std::string ServerPrefix(std::string &server_name)
{
    return ":" + server_name;
}

std::string UserPrefix(Client *client)
{
    return ":" + client->nick_name + "!" + client->user_name + "@" + client->host_name;
}

// NOTE: The caller of this function does *NOT* need to add "\r\n" at the end.
void QueueRawReply(Client *client, const std::string &message)
{
    client->writebuf.append(message + msg::kCrLf);
}

void QueueChannelMessage(EventLoopContext *context, Channel *channel, const std::string &message,
                         const std::map<std::string, Client *> &members, Client *except)
{
    if (!channel)
        return;

    for (std::map<std::string, Client *>::const_iterator it = members.begin(); it != members.end();
         ++it) {
        Client *client = it->second;
        if (client && client != except) {
            QueueRawReply(client, message);
            context->pending_writes.push_back(client->fd);
        }
    }
}

void QueueCommonChannelsMessage(EventLoopContext *context, Client *client,
                                const std::string &message, bool include_self)
{
    if (!client)
        return;

    std::map<int, Client *> recipients;

    for (std::map<std::string, Channel *>::const_iterator cit = client->channels.begin();
         cit != client->channels.end(); ++cit) {
        Channel *channel = cit->second;
        if (!channel)
            continue;

        for (std::map<std::string, Client *>::const_iterator mit = channel->members.begin();
             mit != channel->members.end(); ++mit) {
            Client *common = mit->second;
            if (!common)
                continue;

            if (!include_self && common == client)
                continue;

            recipients[common->fd] = common;
        }
    }

    if (include_self && recipients.find(client->fd) == recipients.end())
        recipients[client->fd] = client;

    for (std::map<int, Client *>::const_iterator it = recipients.begin(); it != recipients.end();
         ++it) {
        QueueRawReply(it->second, message);
        context->pending_writes.push_back(it->second->fd);
    }
}

void QueueNumericReply(Client *client, int numeric, const std::string &message)
{
    if (!client)
        return;

    std::ostringstream oss;

    oss << ":localhost ";

    oss << std::setfill('0') << std::setw(3) << numeric;

    oss << " ";
    if (client->nick_name.empty())
        oss << "*";
    else
        oss << client->nick_name;

    oss << " " << message;
    oss << "\r\n";

    client->writebuf += oss.str();
}

}  // namespace ircs
