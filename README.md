# RDMA Client-Server Implementation

This project implements RDMA (Remote Direct Memory Access) client and server applications using rdma-cm for out-of-band communication with MLX NIC cards.

## Features

- RDMA connection management using rdma-cm
- Reliable Connection (RC) transport
- Memory registration and management
- Send/Receive operations
- Event-driven connection handling

## Prerequisites

### Required Libraries
- libibverbs-dev
- librdmacm-dev
- rdma-core

### Installation
```bash
# Ubuntu/Debian
sudo apt-get install libibverbs-dev librdmacm-dev rdma-core

# RHEL/CentOS
sudo yum install libibverbs-devel librdmacm-devel rdma-core
```

### MLX NIC Configuration
Ensure your MLX NIC cards are properly configured and RDMA subsystem is loaded:
```bash
# Check RDMA devices
ibv_devices

# Check device capabilities
ibv_devinfo
```

## Building

```bash
make all
```

## Usage

### Start the Server
```bash
./rdma_server <port>
# Example: ./rdma_server 12345
```

### Connect with Client
```bash
./rdma_client <server_ip> <port>
# Example: ./rdma_client 192.168.1.100 12345
# Or for local testing: ./rdma_client 127.0.0.1 12345
```

## Architecture

### Server (`rdma_server.cpp`)
- Creates listening endpoint
- Accepts RDMA connections
- Handles connection events
- Performs message exchange

### Client (`rdma_client.cpp`)
- Resolves server address
- Establishes RDMA connection
- Sends messages to server
- Receives responses

## Key Components

- **Protection Domain (PD)**: Memory protection context
- **Completion Queue (CQ)**: Work completion notifications
- **Queue Pair (QP)**: Send/Receive queues
- **Memory Region (MR)**: Registered memory for RDMA operations

## Troubleshooting

1. **Permission Issues**: Ensure user has access to RDMA devices
2. **Module Loading**: Verify RDMA kernel modules are loaded
3. **Network Configuration**: Check firewall and network connectivity
4. **Device Detection**: Use `ibv_devices` to verify MLX cards are detected

## Cleaning

```bash
make clean
```