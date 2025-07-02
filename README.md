# Client-Server Chat Application

A multi-threaded client-server chat application written in C that supports real-time messaging, private messages, user management, and automatic reconnection.

## Features

### Server Features
- **Multi-threaded Architecture**: Handles multiple clients concurrently using pthreads
- **Thread-safe Client Management**: Mutex-protected client list operations
- **Real-time Message Broadcasting**: Instant message delivery to all connected clients
- **Private Messaging**: Direct messages between specific users
- **User Management**: Online user listing and management
- **Connection Timeout**: Automatic disconnection of inactive clients
- **Graceful Shutdown**: Clean server shutdown with proper resource cleanup
- **Logging**: Timestamped server activity logging

### Client Features
- **Interactive Chat Interface**: User-friendly command-line interface
- **Automatic Reconnection**: Attempts to reconnect on connection loss
- **Private Messaging**: Send private messages to specific users
- **User Commands**: Built-in commands for chat management
- **Real-time Message Reception**: Immediate display of incoming messages
- **Graceful Disconnection**: Proper cleanup on exit

## Architecture

### Message Protocol
The application uses a structured message format:
```
type|sender|recipient|content|timestamp
```

Message types:
- `MSG_TEXT`: Regular chat messages
- `MSG_JOIN`: User joining notification
- `MSG_LEAVE`: User leaving notification
- `MSG_LIST_USERS`: Request for user list
- `MSG_PRIVATE`: Private message
- `MSG_ERROR`: Error messages

### Thread Model

**Server Threads:**
- Main accept thread: Accepts new client connections
- Client handler threads: One per connected client
- Timeout handler threads: Monitor client activity

**Client Threads:**
- Main thread: Handles user input
- Receive thread: Processes incoming messages from server

## Building the Application

### Prerequisites
- GCC compiler with C99 support
- POSIX threads (pthread) library
- ncurses library (for client interface)
- Make utility

### Build Commands
```bash
# Build both server and client
make all

# Build server only
make server

# Build client only
make client

# Clean build artifacts
make clean

# Rebuild everything
make rebuild
```

### Installation
```bash
# Install to system directories
make install

# Uninstall from system
make uninstall
```

## Usage

### Starting the Server
```bash
# Run the server
./bin/server

# Or using make
make run-server
```

The server will start listening on port 8080 by default.

### Starting the Client
```bash
# Run the client
./bin/client

# Or using make
make run-client
```

The client will prompt for a username and attempt to connect to localhost:8080.

### Client Commands
- `/help` - Show available commands
- `/users` - List online users
- `/msg <username> <message>` - Send private message
- `/quit` or `/exit` - Leave the chat
- Any other text - Send public message to all users

## Configuration

### Server Configuration
Edit `header.h` to modify:
- `DEFAULT_PORT`: Server port (default: 8080)
- `MAX_CLIENT`: Maximum concurrent clients (default: 10)
- `MAX_MESSAGE_LENGHT`: Maximum message length (default: 1024)
- `CLIENT_TIMEOUT`: Client timeout in seconds (default: 300)

### Client Configuration
Edit `header.h` to modify:
- `SERVER_IP`: Server IP address (default: "127.0.0.1")

## Testing and Debugging

### Memory Leak Detection
```bash
# Check server for memory leaks
make valgrind-server

# Check client for memory leaks
make valgrind-client
```

### Debugging
```bash
# Debug server with GDB
make debug-server

# Debug client with GDB
make debug-client
```

### Testing Multiple Clients
1. Start the server: `./bin/server`
2. Open multiple terminals and run: `./bin/client`
3. Test messaging between clients

## File Structure

```
├── header.h           # Common definitions and structures
├── server.h           # Server-specific declarations
├── client.h           # Client-specific declarations
├── server.c           # Server implementation
├── client.c           # Client implementation
├── server_main.c      # Server main function
├── client_main.c      # Client main function
├── Makefile          # Build configuration
└── README.md         # This documentation
```

## Error Handling

The application includes comprehensive error handling for:
- Socket creation and binding failures
- Connection timeouts and losses
- Thread creation failures
- Memory allocation failures
- Invalid message formats
- Client disconnections

## Security Considerations

- Input validation for all received messages
- Buffer overflow protection
- Timeout mechanisms to prevent resource exhaustion
- Proper resource cleanup to prevent memory leaks

## Limitations

- No encryption (messages sent in plain text)
- No persistent message storage
- No user authentication
- Limited to local network communication
- Maximum message length restriction

## Future Enhancements

- SSL/TLS encryption for secure communication
- User authentication and authorization
- Message history and logging
- File transfer capabilities
- GUI client interface
- Database integration for user management
- Multiple chat rooms/channels

## Troubleshooting

### Common Issues

1. **Port already in use**: Change `DEFAULT_PORT` in `header.h`
2. **Permission denied**: Run with appropriate privileges or use port > 1024
3. **Connection refused**: Ensure server is running and accessible
4. **Compilation errors**: Check that all dependencies are installed

### Debug Mode
Compile with debug flags:
```bash
make CFLAGS="-Wall -Wextra -std=c99 -pthread -g -DDEBUG"
```

## License

This project is provided as-is for educational and development purposes.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## Contact

For questions or issues, please create an issue in the project repository.