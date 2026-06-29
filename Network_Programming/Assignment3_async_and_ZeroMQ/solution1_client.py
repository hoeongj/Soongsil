import asyncio
import sys

# Client configuration — must match server HOST and PORT
HOST = '127.0.0.1'
PORT = 9000


# TODO: Implement the chat client.
#
# - Connect to the server, join a channel, and chat.
# - Must handle network input and user input concurrently.
# - Typing "quit" disconnects.
#
# Hint: Reference Chapter 7 for asyncio client patterns.


async def receive_messages(reader):
    # This coroutine is responsible only for network input.
    # Keeping it separate from keyboard input gives simultaneous I/O.
    # Receive messages from the server while the user types input.
    # This coroutine runs as a separate asyncio task.
    try:
        while True:
            # Read one complete line from the server protocol.
            data = await reader.readline()
            if not data:
                # Empty bytes mean the server closed the connection.
                print('Disconnected from server.', flush=True)
                break

            # Remove the line ending before showing the message.
            message = data.decode('UTF-8').rstrip('\r\n')
            print(message, flush=True)

    except asyncio.CancelledError:
        # Cancellation is normal when the client is shutting down.
        raise
    except UnicodeDecodeError:
        print('Received invalid UTF-8 from server.', flush=True)
    except (ConnectionError, OSError, RuntimeError) as e:
        print('Network error: ' + str(e), flush=True)


async def send_user_messages(writer):
    # This coroutine is responsible only for keyboard input and sending.
    # stdin.readline is blocking, so it is moved to a background thread.
    # This keeps the receive task running at the same time.
    try:
        while True:
            # The prompt is printed locally. It is not sent to the server.
            print('> ', end='', flush=True)
            line = await asyncio.to_thread(sys.stdin.readline)
            if line == '':
                # EOF from the terminal is handled the same way as quit.
                line = 'quit\n'

            message = line.rstrip('\r\n')
            # The server expects every message to end with one newline.
            writer.write((message + '\n').encode('UTF-8'))
            await writer.drain()

            # Send quit to the server first, then close the client side.
            if message == 'quit':
                # The loop stops only after the quit command is delivered.
                break

    except asyncio.CancelledError:
        raise
    except (ConnectionError, OSError, RuntimeError) as e:
        print('Network error: ' + str(e), flush=True)


async def main():
    # This block prepares the channel name before opening the socket.
    # The channel name must be sent as the first line after connecting.
    channel = input('Channel: ').strip()
    if channel == '':
        # Empty channel names are rejected locally to avoid invalid joins.
        print('Channel name required.', flush=True)
        return

    # This block opens the TCP connection to the chat server.
    reader, writer = await asyncio.open_connection(HOST, PORT)
    receive_task = None

    try:
        # Join phase:
        # Send the selected channel first because the server expects it.
        # Send the join request before starting normal chat.
        writer.write((channel + '\n').encode('UTF-8'))
        await writer.drain()

        # Chat phase:
        # One task receives server messages while this coroutine reads stdin.
        # Receive from the network and read from the terminal concurrently.
        receive_task = asyncio.create_task(receive_messages(reader))
        await send_user_messages(writer)

    finally:
        # Cleanup phase:
        # Stop the receive task and close the socket on every exit path.
        if receive_task is not None:
            receive_task.cancel()
            try:
                # Wait for cancellation so the task finishes cleanly.
                await receive_task
            except asyncio.CancelledError:
                pass

        writer.close()
        await writer.wait_closed()


if __name__ == '__main__':
    # This block is the script entry point for running the client directly.
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print('\nClient stopped.', flush=True)
