# TCP Server-Client System for Process Simulation

## Overview
This project consists of a **TCP Server** and a **TCP Client** that simulate a dynamic process generation and handling system. The client generates random processes, sends them to the server, and receives acknowledgments. The server processes the data and provides responses.

## Features
### TCP Server
- Listens for client connections on a configurable IP address and port.
- Reads and parses incoming data representing processes.
- Sends acknowledgments or responses to the client after processing.

### TCP Client
- Generates random processes with unique IDs and execution times.
- Connects to the server and sends the generated processes as a single message.
- Receives and prints server responses.
- Operates continuously to simulate real-time activity.

---

## Configuration
Both the server and client use the same IP address and port for communication. Update these values in both files to match your setup.

### Configuration Macros
In the client and server source code:
```c
#define SERVER_ADDRESS  "0.0.0.0"
#define PORT            8080
