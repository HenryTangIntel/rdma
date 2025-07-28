CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g -O0 -DDEBUG
LDFLAGS = -lrdmacm -libverbs -lpthread

SRCDIR = .
SOURCES = rdma_server.cpp rdma_client.cpp
TARGETS = rdma_server rdma_client

.PHONY: all clean

all: $(TARGETS)

rdma_server: rdma_server.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

rdma_client: rdma_client.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS)

install:
	@echo "Make sure you have the following packages installed:"
	@echo "  - libibverbs-dev"
	@echo "  - librdmacm-dev" 
	@echo "  - rdma-core"
	@echo ""
	@echo "On Ubuntu/Debian: sudo apt-get install libibverbs-dev librdmacm-dev rdma-core"
	@echo "On RHEL/CentOS: sudo yum install libibverbs-devel librdmacm-devel rdma-core"

test: all
	@echo "To test the RDMA communication:"
	@echo "1. Start the server: ./rdma_server <port>"
	@echo "2. In another terminal, start the client: ./rdma_client <server_ip> <port>"
	@echo "Example: ./rdma_server 12345"
	@echo "         ./rdma_client 127.0.0.1 12345"