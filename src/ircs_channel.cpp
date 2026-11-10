#include "ircs_channel.h"

#include <map>
#include <sstream>

#include "ircs_channel.h"
#include "ircs_client.h"
#include "ircs_command.h"
#include "ircs_logger.h"

namespace ircs {
using log::Logger;

// NOTE: As specified in RFC 2812 Section 2.2:
//       {}|^ are lower case equivalents of []\~

// clang-format off
/* lowertab[c] = lowercase of c */
const unsigned char tolowertab[256] = {
/* 0x00 */ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07, 0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
/* 0x10 */ 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17, 0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
/* 0x20 */ ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/',
/* 0x30 */ '0', '1', '2', '3', '4', '5', '6', '7',  '8', '9', ':', ';', '<', '=', '>', '?',
/* 0x40 */ '@', 'a', 'b', 'c', 'd', 'e', 'f', 'g',  'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
/* 0x50 */ 'p', 'q', 'r', 's', 't', 'u', 'v', 'w',  'x', 'y', 'z', '{', '|', '}', '^', '_',
/* 0x60 */ '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g',  'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
/* 0x70 */ 'p', 'q', 'r', 's', 't', 'u', 'v', 'w',  'x', 'y', 'z', '{', '|', '}', '^', 0x7F,
/* 0x80 */ 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87, 0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
/* 0x90 */ 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97, 0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
/* 0xA0 */ 0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7, 0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
/* 0xB0 */ 0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7, 0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
/* 0xC0 */ 0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7, 0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
/* 0xD0 */ 0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7, 0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
/* 0xE0 */ 0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7, 0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
/* 0xF0 */ 0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7, 0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
};

/* uppertab[c] = uppercase of c */
const unsigned char touppertab[256] = {
/* 0x00 */ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07, 0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
/* 0x10 */ 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17, 0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
/* 0x20 */ ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/',
/* 0x30 */ '0', '1', '2', '3', '4', '5', '6', '7',  '8', '9', ':', ';', '<', '=', '>', '?',
/* 0x40 */ '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',  'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
/* 0x50 */ 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',  'X', 'Y', 'Z', '[', '\\', ']', '~', '_',
/* 0x60 */ '`', 'A', 'B', 'C', 'D', 'E', 'F', 'G',  'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
/* 0x70 */ 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',  'X', 'Y', 'Z', '[', '\\', ']', '~', 0x7F,
/* 0x80 */ 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87, 0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
/* 0x90 */ 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97, 0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
/* 0xA0 */ 0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7, 0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
/* 0xB0 */ 0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7, 0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
/* 0xC0 */ 0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7, 0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
/* 0xD0 */ 0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7, 0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
/* 0xE0 */ 0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7, 0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
/* 0xF0 */ 0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7, 0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
};
// clang-format on

int GetChannelName(std::string recipients[kTargMax], const std::string &s)
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

// Since Channel Names in IRC are case insensitive the name will be lowercased,
// including the characters '{}|^' (or the upper case variants '[]\~')
std::string NormalizeChannelName(const std::string &name)
{
    std::string normalized = name;
    for (size_t i = 0; i < normalized.length(); i++) {
        unsigned char c = normalized[i];
        normalized[i] = tolowertab[c];
    }
    return normalized;
}

// returns empty if unsuccessful
std::string ParseChannelName(std::string &name)
{
    size_t pos = 0;
    size_t end = name.length();

    // The RFC 2812 specifies that channel names can't contain either KOMMA,
    // SPACE OR BEL Character.
    // However, we only need to check for BEL character since ',' and ' '
    // are already handled due to how this server parses messages from the
    // client
    if (name.find('\a') != name.npos) {
        return "";
    }

    std::string channel_name = NormalizeChannelName(name);

    if (name[0] == '#') {
        pos = 1;
    }

    if (name.length() - pos > kMaxChannelNameSize) {
        end = kMaxChannelNameSize;
    }

    return std::string(channel_name, pos, end);
}

Channel *FindChannel(const std::map<std::string, Channel *> &channels, const std::string &name)
{
    std::string channel_name = NormalizeChannelName(name);
    std::map<std::string, Channel *>::const_iterator it = channels.find(channel_name);

    if (it == channels.end()) {
        return NULL;
    }

    return it->second;
}

Channel *CreateChannel(std::string &channel_name, Client *client)
{
    Channel *chan = new Channel;

    chan->joined = 0;
    chan->name = channel_name;
    chan->max_members = kDefaultMaxMembers;
    MakeChannelOperator(*chan, client);
    Logger(log::kInfo, "", "creating channel %s", channel_name.c_str());
    return chan;
}

// @returns the number of members in the channel on success; 0 if the member is
//          already in the channel; -1 if the channel is full
int AddMemberToChannel(Channel &channel, Client *client)
{
    if (channel.joined == channel.max_members) {
        return -1;
    }
    if (channel.members.find(client->nick_name) != channel.members.end()) {
        return 0;
    }

    Logger(log::kInfo, "", "#%s: %s joined", channel.name.c_str(), client->nick_name.c_str());
    channel.members[client->nick_name] = client;
    channel.joined += 1;
    client->channels[channel.name] = &channel;
    return channel.joined;
}

bool IsChannelOperator(const Channel &channel, const std::string &nick)
{
    if (channel.operators.find(nick) == channel.operators.end()) {
        return false;
    }
    return true;
}

bool IsOnChannel(const Channel &channel, const std::string &nick) {
    if (channel.members.find(nick) == channel.members.end()) {
        return false;
    }
    return true;
}

// If the client is not yet a member of the channel, this function will also add
// it
int MakeChannelOperator(Channel &channel, Client *client)
{
    if (channel.operators.find(client->nick_name) != channel.operators.end()) {
        return 0;
    }
    if (channel.members.find(client->nick_name) == channel.members.end()) {
        if (AddMemberToChannel(channel, client) == -1) {
            return -1;
        }
    }

    Logger(log::kInfo, "", "#%s: %s is now an operator", channel.name.c_str(),
           client->nick_name.c_str());
    channel.operators[client->nick_name] = client;
    return 0;
}

void RemoveChannelOperator(Channel &channel, Client *client)
{
    std::map<std::string, Client *>::iterator it;

    it = channel.operators.find(client->nick_name);
    if (it == channel.operators.end()) {
        return;
    }

    Logger(log::kInfo, "", "#%s: %s's operator priviliges have been revoked", channel.name.c_str(),
           client->nick_name.c_str());
    channel.operators.erase(it);
}

void RemoveMemberFromChannel(std::map<std::string, Channel *> channels, Channel &channel,
                             Client *client)
{
    std::map<std::string, Client *>::iterator it;

    RemoveChannelOperator(channel, client);

    it = channel.members.find(client->nick_name);
    if (it != channel.members.end()) {
        Logger(log::kInfo, "", "#%s: %s has been removed", channel.name.c_str(),
               client->nick_name.c_str());
        channel.members.erase(it);
    }

    if (channel.joined > 0) {
        channel.joined -= 1;
    }

    // TODO: move this logic to the calling function
    if (channel.joined == 0) {
        delete channels[channel.name];
        channels.erase(channel.name);
    }
    return;
}

// Gets a string of all nicknames in the list.
// Also adds '@' to the nicknames that have operator priviliges
std::string GetNicknameList(Channel &channel)
{
    std::string list;
    std::map<std::string, Client *>::iterator it = channel.members.begin();
    while (it != channel.members.end()) {
        if (IsChannelOperator(channel, it->second->nick_name)) {
            list += '@';
        }
        list += it->second->nick_name;
        ++it;
        if (it != channel.members.end()) {
            list += " ";
        }
    }
    Logger(log::kDebug, "", "List of #%s: %s", channel.name.c_str(), list.c_str());
    return list;
}

}  // namespace ircs
