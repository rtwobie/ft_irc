#include "ircs_client.h"

#include <sys/socket.h>

#include <vector>

#include "ircs_event_loop.h"
#include "ircs_logger.h"

namespace ircs {
using log::Logger;

Client *NewClient(int fd, char *ip)
{
    Client *c = new Client();

    c->fd = fd;
    c->ip = ip;
    c->message_buffer = msg::MessageBuffer();
    c->writebuf.reserve(msg::kMaxMsgSize);
    c->write_pos = 0;

    c->state = kClientConnected;

    c->pass_accepted = false;
    c->nick_accepted = false;
    c->name_accepted = false;

    return c;
}

void OnClientRead(EventLoop *event_loop, int fd, void *data, int mask)
{
    EventLoopContext *context = &event_loop->context;
    Client *client = static_cast<Client *>(data);
    ssize_t bytes_received;
    char buf[kReadChunkSize];

    bytes_received = recv(fd, buf, sizeof(buf), 0);
    if (bytes_received == -1) {
        return;
    }

    if (bytes_received == 0) {
        CloseConnection(event_loop, fd, client);
        return;
    }

    if (bytes_received > 0) {
        Logger(log::kDebug, "", "received: %s", buf);
        msg::ProcessStatus status = msg::ProcessTcpStream(context, buf, client, bytes_received);

        if (!client->writebuf.empty() && !(mask & kOnWritable)) {
            if (CreateFileEvent(event_loop, fd, kOnWritable, OnClientWrite, data) == -1) {
                CloseConnection(event_loop, fd, client);
                return;
            }
        }

        std::vector<int> &pending_writes = context->pending_writes;
        for (size_t i = 0; i < pending_writes.size(); i++) {
            Client *other = context->clients[pending_writes[i]];
            if (!other->writebuf.empty() && !(mask & kOnWritable)) {
                if (CreateFileEvent(event_loop, other->fd, kOnWritable, OnClientWrite, other) == -1) {
                    CloseConnection(event_loop, other->fd, other);
                }
            }
        }
        pending_writes.clear();

        if (status == msg::kCloseConnection || status == msg::kProtocolError) {
            RequestClose(event_loop, fd, mask, client);
        }
    }
}

void OnClientWrite(EventLoop *event_loop, int fd, void *data, int mask)
{
    (void)mask;
    Client *client = static_cast<Client *>(data);
    ssize_t bytes_sent;
    size_t bytes_to_sent = client->writebuf.length() - client->write_pos;

    if (bytes_to_sent == 0) {
        RemoveFileEvent(event_loop, fd, kOnWritable, client);
        if (event_loop->events[fd].state == kFileEventClosing) {
            CloseConnection(event_loop, fd, client);
        }
        return;
    }

    bytes_sent =
        send(fd, client->writebuf.c_str() + client->write_pos, bytes_to_sent, MSG_NOSIGNAL);
    if (bytes_sent == -1) {
        return;
    }

    client->write_pos += bytes_sent;
    if (client->write_pos == client->writebuf.length()) {
        client->writebuf.clear();
        client->write_pos = 0;
        RemoveFileEvent(event_loop, fd, kOnWritable, client);
        if (event_loop->events[fd].state == kFileEventClosing) {
            CloseConnection(event_loop, fd, client);
            return;
        }
    }
}

}  // namespace ircs
