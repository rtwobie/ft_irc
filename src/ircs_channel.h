#ifndef IRCS_CHANNEL_H_
#define IRCS_CHANNEL_H_

#include <stdint.h>

#include <map>
#include <string>

namespace ircs {

struct Client;

const int kDefaultMaxMembers = 10;
const int kMaxChannelNameSize = 50;
const int kMaxJoinedChannels = 2;  // NOTE: 2 for now; makes testing easier

const char kChanModeInviteOnlyIdentifier = 'i';
const char kChanModeTopicIdentifier = 't';
const char kChanModeKeyIdentifier = 'k';
const char kChanModeOperatorIdentifier = 'o';
const char kChanModeLimitIdentifier = 'l';

const uint32_t kChanModeInviteOnly = 0x0001;
const uint32_t kChanModeTopic = 0x0002;
const uint32_t kChanModeKey = 0x0004;
// const uint32_t kChanModeOperator = 0x0008; // Channel can't be of type 'o';
const uint32_t kChanModeLimit = 0x0010;

struct ChannelMode {
    uint32_t mask;
    char identifier;
};

struct Channel {
    size_t max_members;
    std::string name;
    std::string topic;
    std::string key;
    std::map<std::string, Client *> operators;
    std::map<std::string, Client *> members;
    std::map<std::string, Client *> invited;
    uint32_t modes;
};

std::string ParseChannelName(const std::string &name);
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

// Channel Mode
std::string GetChannelModesAndParams(Channel &channel);
int AddChannelMode(Channel &channel, char identifier);
void AddChannelMode(Channel &channel, int mask);

}  // namespace ircs

#endif  // IRCS_CHANNEL_H_
