#include "ircs_command.h"

#include <sstream>
#include <vector>

#include "ircs_client.h"
#include "ircs_client_io.h"
#include "ircs_event_loop.h"
#include "ircs_logger.h"

namespace ircs {
using log::Logger;

namespace {

void SendWelcome(Client *client, EventLoopContext *context)
{
    if (!client)
        return;

    const std::string full_client_id =
        client->nick_name + "!" + client->user_name + "@" + client->host_name;

    const std::string rpl_welcome = ":Welcome to the Internet Relay Network " + full_client_id;
    QueueNumericReply(client, kRplWelcome, rpl_welcome);

    const std::string rpl_yourhost =
        ":Your host is " + context->host_name + ", running version " + context->version;
    QueueNumericReply(client, kRplYourHost, rpl_yourhost);

    const std::string rpl_created = "This server was created " + context->creation_date;
    QueueNumericReply(client, kRplCreated, rpl_created);

    const std::string rpl_myinfo = context->host_name + " " + context->version + " io itkol";
    QueueNumericReply(client, kRplMyInfo, rpl_myinfo);
}

CommandStatus HandlePass(const CommandEntry *command, EventLoopContext *context, Client *client,
                         const msg::Message &message)
{
    if (client->state == kClientRegistered) {
        QueueNumericReply(client, kErrAlreadyRegistered, ":You may not reregister");
        return kContinue;
    }
    if (message.param_count < command->min_params) {
        QueueNumericReply(client, kErrNeedMoreParams, "PASS :Not enough parameters");
        return kContinue;
    }
    if (message.parameters[0] != context->password) {
        QueueNumericReply(client, kErrPasswdMismatch, ":Password incorrect");
        return kDisconnect;
    }

    client->pass_accepted = true;

    return kContinue;
}

CommandStatus HandleNick(const CommandEntry *command, EventLoopContext *context, Client *client,
                         const msg::Message &message)
{
    if (message.param_count < command->min_params) {
        QueueNumericReply(client, kErrNoNicknameGiven, ":No nickname given");
        return kContinue;
    }

    const std::string nick_name = message.parameters[0];
    char first = nick_name[0];

    if (first == '$' || first == ':' || first == '#' || first == '&' || first == '@' ||
        first == '+') {
        QueueNumericReply(client, kErrErroneousNickname, " :Erroneous nickname");
        return kContinue;
    }

    if (nick_name.empty() || nick_name.length() > 30 || nick_name.length() < 2) {
        QueueNumericReply(client, kErrErroneousNickname, " :Erroneous nickname");
        return kContinue;
    }

    for (size_t i = 0; i < nick_name.length(); ++i) {
        char c = nick_name[i];
        if (c == ' ' || c == ',' || c == '*' || c == '?' || c == '!' || c == '@') {
            QueueNumericReply(client, kErrErroneousNickname, " :Erroneous nickname");
            return kContinue;
        }
    }

    for (std::map<int, Client *>::iterator it = context->clients.begin();
         it != context->clients.end(); ++it) {
        if (it->second->fd != client->fd) {
            if (it->second->nick_name == nick_name) {
                QueueNumericReply(client, kErrNicknameInUse, " :Nickname is already in use");
                return kContinue;
            }
        }
    }

    if (client->state == kClientHasUser && client->state != kClientRegistered) {
        client->nick_name = nick_name;
        client->state = kClientRegistered;
        SendWelcome(client, context);
        return kContinue;
    } else if (client->state == kClientConnected) {
        client->state = kClientHasNick;
    }

    // Notify client on nickname change
    if (!client->nick_name.empty()) {
        std::string reply = ":" + client->nick_name + "!" + client->user_name + "@" +
                            client->host_name + " " + command->name + " " + nick_name;
        QueueRawReply(client, reply);
    }

    client->nick_name = nick_name;

    return kContinue;
}

CommandStatus HandleUser(const CommandEntry *command, EventLoopContext *context, Client *client,
                         const msg::Message &message)
{
    if (client->state == kClientRegistered) {
        QueueNumericReply(client, kErrAlreadyRegistered, ":You may not reregister");
        return kContinue;
    }
    if (message.param_count < command->min_params) {
        QueueNumericReply(client, kErrNeedMoreParams, "PASS :Not enough parameters");
        return kContinue;
    }

    std::string user_name = message.parameters[0];
    const std::string real_name = message.parameters[3];

    if (user_name.length() > 10) {
        user_name = user_name.substr(0, 10);
    }

    if (client->state == kClientHasNick && client->state != kClientRegistered) {
        client->user_name = user_name;
        client->host_name = "localhost";  // TO DO: change the hostname
        client->real_name = real_name;
        client->state = kClientRegistered;
        SendWelcome(client, context);
        return kContinue;
    } else if (client->state == kClientConnected) {
        client->state = kClientHasUser;
    }

    client->user_name = user_name;
    client->host_name = "localhost";  // TO DO: change the hostname
    client->real_name = real_name;

    return kContinue;
}

// NOTE: We don't implement it but are required to respond to the client
//       with ERR_SUMMONDISABLED (see RFC 2812 section 5.2)
CommandStatus HandleSummon(const CommandEntry *command, EventLoopContext *context, Client *client,
                           const msg::Message &message)
{
    (void)context;
    (void)message;
    (void)command;
    QueueNumericReply(client, kErrSummonDisabled, ":SUMMON has been disabled");
    return kContinue;
}

// NOTE: We don't implement it but are required to respond to the client
//       with ERR_USERSDISABLED(see RFC 2812 section 5.2)
CommandStatus HandleUsers(const CommandEntry *command, EventLoopContext *context, Client *client,
                          const msg::Message &message)
{
    (void)context;
    (void)message;
    (void)command;
    QueueNumericReply(client, kErrUsersDisabled, ":USERS has been disabled");
    return kContinue;
}

CommandStatus HandleQuit(const CommandEntry *command, EventLoopContext *context, Client *client,
                         const msg::Message &message)
{
    (void)command;
    (void)context;
    std::string reply;

    // TODO: send quit message to all clients that are in the same channel as
    //       the one quitting

    reply = "ERROR :Closing Link: " + client->user_name + "@" + client->host_name + " (" +
            message.parameters[0] + ")";
    QueueRawReply(client, reply);
    return kContinue;
}

// TODO: We need to implement this when we implement the Ping logic
// CommandStatus HandlePong(const CommandEntry *command, EventLoopContext *context, Client *client,
//                          const msg::Message &message)
// {
//     // Because our server doesnt support a server-to-server network we simply
//     // send ERR_NOSUCHSERVER
//     if (message.param_count > 1 && message.has_trailing == false) {
//         QueueNumericReply(client, kErrNoSuchServer, message.parameters[1] + " :No such server");
//         return kContinue;
//     }
//
// }

bool IsLocalHost(const std::string &name)
{
    return (name == "localhost" || name == "127.0.0.1") ? true : false;
}

CommandStatus HandlePing(const CommandEntry *command, EventLoopContext *context, Client *client,
                         const msg::Message &message)
{
    std::string reply;

    if (message.param_count < command->min_params) {
        QueueNumericReply(client, kErrNoOrigin, ":No origin specified");
        return kContinue;
    }

    for (int i = 0; i < message.param_count && i < command->max_params; i++) {
        if (IsLocalHost(message.parameters[i]) && IsLocalHost(context->host_name)) {
            continue;
        }
        if (message.parameters[i] != context->host_name) {
            reply = message.parameters[i] + " :No such server";
            QueueNumericReply(client, kErrNoSuchServer, reply);
            return kContinue;
        }
    }

    reply = ":" + context->host_name + " PONG " + context->host_name;
    QueueRawReply(client, reply);

    return kContinue;
}

int GetRecipients(std::string recipients[kTargMax], const std::string &s)
{
    std::stringstream ss(s);
    std::string recipient;
    const char delim = ',';

    int i = 0;
    while (std::getline(ss, recipient, delim)) {
        if (i >= kTargMax) {
            return -1;
        }
        recipients[i] = recipient;
        i++;
    }
    return 0;
}

Client *FindClient(std::map<int, Client *> &clients, const std::string &nickname)
{
    std::map<int, Client *>::const_iterator it = clients.begin();

    while (it != clients.end()) {
        if (it->second->nick_name == nickname) {
            return clients[it->first];
        }
        it++;
    }
    return NULL;
}

CommandStatus HandlePrivMsg(const CommandEntry *command, EventLoopContext *context, Client *client,
                            const msg::Message &message)
{
    // NOTE: Possible Error codes
    // ERR_NORECIPIENT
    // ERR_NOTEXTTOSEND
    // ERR_TOOMANYTARGETS
    //
    // ERR_CANNOTSENDTOCHAN
    // ERR_WILDTOPLEVEL
    // ERR_NOSUCHNICK
    // ERR_NOTOPLEVEL
    // RPL_AWAY

    if (client->state != kClientRegistered) {
        QueueNumericReply(client, kErrNotRegistered, ":You have not registered");
        return kContinue;
    }

    if (message.parameters[0].empty()) {
        QueueNumericReply(client, kErrNoRecipient, ":No recipient given (" + command->name + ")");
        return kContinue;
    }

    if (message.has_trailing == false) {
        QueueNumericReply(client, kErrNoTextToSend, ":No text to send");
        return kContinue;
    } else if (message.has_trailing == true &&
               message.parameters[command->min_params - 1].empty()) {
        QueueNumericReply(client, kErrNoTextToSend, ":No text to send");
        return kContinue;
    }

    // Send the incoming message to the recipients, but add client credentials as a prefix
    std::string sender_prefix =
        client->nick_name + "!" + client->user_name + "@" + client->host_name;
    std::string reply = sender_prefix + " " + command->name + " " + message.parameters[0] + " :" +
                        message.parameters[1];

    std::string recipients[kTargMax];
    if (GetRecipients(recipients, message.parameters[0]) == -1) {
        std::ostringstream oss;
        oss << ":Too many recipients. Only the first ";
        oss << kTargMax;
        oss << " will be processed.";
        QueueNumericReply(client, kErrTooManyTargets, oss.str());
        return kContinue;
    }

    for (size_t i = 0; i < kTargMax; i++) {
        // WARN: CURRENT LIMITATION:
        //       Trying to send a message to a channel will report ERR_NOSUCHCHANNEL
        //       as Channels are not implemented yet
        if (recipients[i][0] == '#') {
            QueueNumericReply(client, kErrNoSuchChannel, recipients[i] + " :No such channel");
            continue;
        }
        Client *recipient = FindClient(context->clients, recipients[i]);
        if (recipient == NULL) {
            QueueNumericReply(client, kErrNoSuchNick, recipients[i] + " :No such nick/channel");
            continue;
        }

        QueueRawReply(recipient, reply);
        context->pending_writes.push_back(recipient->fd);
    }
    return kContinue;
}

// NOTE: Add Commands in this table!
std::vector<CommandEntry> InitializeCommandTable()
{
    std::vector<CommandEntry> table;

    table.push_back((CommandEntry){kPass, HandlePass, 1, 1});
    table.push_back((CommandEntry){kUser, HandleUser, 4, 4});
    table.push_back((CommandEntry){kNick, HandleNick, 1, 1});
    table.push_back((CommandEntry){kSummon, HandleSummon, 0, 0});
    table.push_back((CommandEntry){kUsers, HandleUsers, 0, 0});
    table.push_back((CommandEntry){kPrivMsg, HandlePrivMsg, 2, 2});
    table.push_back((CommandEntry){kPing, HandlePing, 1, 2});
    table.push_back((CommandEntry){kQuit, HandleQuit, 0, 1});

    return table;
}

const static std::vector<CommandEntry> kCommandTable = InitializeCommandTable();

void NormalizeCommandName(std::string &command)
{
    for (size_t i = 0; i < command.length(); i++) {
        command[i] = std::toupper(command[i]);
    }
}

const CommandEntry *LookupCommand(const std::string &command)
{
    for (size_t i = 0; i < kCommandTable.size(); i++) {
        if (command == kCommandTable[i].name) {
            return &kCommandTable[i];
        }
    }
    return NULL;
}

}  // namespace

msg::ProcessStatus DispatchCommand(EventLoopContext *context, Client *client, msg::Message &message)
{
    NormalizeCommandName(message.command);
    const CommandEntry *cmd = LookupCommand(message.command);
    if (cmd == NULL) {
        QueueNumericReply(client, kErrUnknownCommand, message.command + " :Unknown command");
        return msg::kOk;
    }

    Logger(log::kDebug, "", "executing command %s...", message.command.c_str());
    CommandStatus status = cmd->handler(cmd, context, client, message);

    if (status == kDisconnect) {
        return msg::kCloseConnection;  // TO DO: new msg for failure
    }
    return msg::kOk;
}

}  // namespace ircs
