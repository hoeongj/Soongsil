import asyncio

# Server configuration
HOST = '127.0.0.1'
PORT = 9000


# TODO: Implement the async chat relay server.
#
# - Manage named channels and relay messages between
#   clients in the same channel.
# - Handle client joins, departures, and unexpected
#   disconnections gracefully.
# - All messages are UTF-8 text terminated by '\n'.
#
# Hint: Reference Chapter 7 for asyncio server patterns.


if __name__ == '__main__':
    pass


# Implementation starts here. The original template above is kept unchanged.
# The server is divided into small helper functions:
# 1. get_peer_name() makes a readable client name.
# 2. send_line() sends one line using the assignment protocol.
# 3. broadcast() relays messages inside one channel.
# 4. remove_client() cleans up channel membership.
# 5. handle_client() controls one full client session.

# Each channel name maps to the set of StreamWriter objects in that room.
# Example: channels['study'] contains all clients in the study room.
channels = {}


def get_peer_name(writer):
    # This helper is only for display/logging, not for protocol logic.
    # Use the TCP peer address for server logs and forwarded messages.
    peer = writer.get_extra_info('peername')
    if peer is None:
        # peername can be missing if the connection is already closed.
        return 'unknown'
    return str(peer[0]) + ':' + str(peer[1])


async def send_line(writer, message):
    # This helper keeps the newline and UTF-8 encoding rule in one place.
    # Every protocol message is sent as one UTF-8 line.
    writer.write((message + '\n').encode('UTF-8'))
    # drain() waits until the queued bytes are actually written.
    await writer.drain()


async def broadcast(channel, message, sender=None):
    # This block implements the main relay rule of Question 1.
    # Send the message to all clients in the same channel except the sender.
    # list() is used so the set can be changed safely during iteration.
    # This also prevents messages from leaking into a different channel.
    for writer in list(channels.get(channel, set())):
        # The sender should not receive its own chat message.
        if writer is sender:
            continue
        try:
            await send_line(writer, message)
        except (ConnectionError, OSError, RuntimeError):
            # If one client is already gone, remove only that client.
            channels.get(channel, set()).discard(writer)

    # If failed sends removed the last client, delete the empty room.
    if channel in channels and len(channels[channel]) == 0:
        del channels[channel]


async def remove_client(writer, channel):
    # This block removes one connection from the channel table.
    # This function is used for both normal quit and unexpected disconnect.
    peer_name = get_peer_name(writer)
    members = channels.get(channel)

    # Only notify other clients if this writer was really in the channel.
    if members is not None and writer in members:
        members.remove(writer)
        if len(members) == 0:
            # Empty channel entries are removed to keep the dictionary clean.
            del channels[channel]

        print('LEAVE ' + peer_name + ' <- ' + channel, flush=True)
        await broadcast(channel, '[' + peer_name + '] left the channel')

    # This block closes the TCP connection even if cleanup above failed.
    try:
        writer.close()
        await writer.wait_closed()
    except (ConnectionError, OSError, RuntimeError):
        pass


async def handle_client(reader, writer):
    # This coroutine handles exactly one connected client.
    # asyncio.start_server calls this function for each accepted connection.
    peer_name = get_peer_name(writer)
    # channel stays None until the first line is successfully read.
    channel = None

    try:
        # Join phase:
        # The server waits for the first protocol line and treats it as
        # the channel name requested by the client.
        # The first line from the client is the channel name.
        data = await reader.readline()
        if not data:
            # The client closed the socket before joining any channel.
            return

        channel = data.decode('UTF-8').strip()
        if channel == '':
            # A client must join a valid channel before sending chat messages.
            await send_line(writer, 'Channel name required')
            return

        channels.setdefault(channel, set()).add(writer)
        print('JOIN ' + peer_name + ' -> ' + channel, flush=True)
        await send_line(writer, 'Joined channel: ' + channel)

        # Chat phase:
        # This loop keeps reading messages until quit or disconnection.
        # After joining, all later lines are chat messages or quit.
        while True:
            data = await reader.readline()
            if not data:
                # readline() returns empty bytes when the client disconnects.
                print('DISCONNECT ' + peer_name + ' from ' + channel, flush=True)
                break

            message = data.decode('UTF-8').rstrip('\r\n')
            if message == 'quit':
                # quit is treated as a graceful disconnect command.
                print('QUIT ' + peer_name + ' from ' + channel, flush=True)
                break

            print('MESSAGE ' + peer_name + ' [' + channel + ']: ' + message,
                  flush=True)
            # Normal chat messages are forwarded to other clients only.
            await broadcast(channel, '[' + peer_name + '] ' + message, writer)

    except UnicodeDecodeError:
        # Invalid bytes should not crash the whole server.
        print('Invalid UTF-8 from ' + peer_name, flush=True)
    except (ConnectionError, OSError, RuntimeError) as e:
        # Socket errors are logged, then the finally block cleans up.
        print('Connection error from ' + peer_name + ': ' + str(e), flush=True)
    finally:
        # Cleanup phase:
        # Every exit path goes through here, so channel state stays correct.
        if channel is not None:
            await remove_client(writer, channel)
        else:
            # If the client never joined, just close the socket.
            try:
                writer.close()
                await writer.wait_closed()
            except (ConnectionError, OSError, RuntimeError):
                pass


async def main():
    # This block starts the TCP server and keeps it alive.
    # asyncio.start_server creates one task for each connected client.
    server = await asyncio.start_server(handle_client, HOST, PORT)
    print('Server listening on ' + HOST + ':' + str(PORT), flush=True)

    async with server:
        # serve_forever() runs until KeyboardInterrupt stops asyncio.run().
        await server.serve_forever()


if __name__ == '__main__':
    # This block is the script entry point for running the server directly.
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print('Server stopped.', flush=True)
