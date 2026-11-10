#ifndef IRCS_CHANNEL_H_
#define IRCS_CHANNEL_H_

#include <map>
#include <string>

namespace ircs {

struct Client;

const int kDefaultMaxMembers = 10;
const int kMaxChannelNameSize = 50;
const int kMaxJoinedChannels = 2;  // NOTE: 2 for now; makes testing easier

struct Channel {
    size_t max_members;
    std::string name;
    std::string topic;
    std::map<std::string, Client *> operators;
    std::map<std::string, Client *> members;
};

std::string ParseChannelName(std::string &name);
Channel *FindChannel(const std::map<std::string, Channel *> &channels, const std::string &name);
Channel *CreateChannel(std::string &channel_name, Client *client);
int AddMemberToChannel(Channel &channel, Client *client);
void RemoveMemberFromChannel(Channel &channel, Client *client);
void DeleteChannel(std::map<std::string, Channel *> &channels, Channel *channel);
int MakeChannelOperator(Channel &channel, Client *client);
void RemoveChannelOperator(Channel &channel, Client *client);
bool IsChannelOperator(const Channel &channel, const std::string &nick);
bool IsOnChannel(const Channel &channel, const std::string &nick);
std::string GetNicknameList(Channel &channel);

}  // namespace ircs

#endif  // IRCS_CHANNEL_H_
