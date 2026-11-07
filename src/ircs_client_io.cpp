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
