CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g -O0 -DDEBUG
LDFLAGS = -lrdmacm -libverbs -lpthread

SRCDIR = .
SOURCES = rdma_server.cpp rdma_client.cpp tcp_server.cpp tcp_client.cpp
RDMA_TARGETS = rdma_server rdma_client
TCP_TARGETS = tcp_server tcp_client
ALL_TARGETS = $(RDMA_TARGETS) $(TCP_TARGETS)

.PHONY: all clean rdma tcp

all: $(ALL_TARGETS)

rdma: $(RDMA_TARGETS)

tcp: $(TCP_TARGETS)

rdma_server: rdma_server.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

rdma_client: rdma_client.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

tcp_server: tcp_server.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< -lpthread

tcp_client: tcp_client.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< -lpthread

clean:
	rm -f $(ALL_TARGETS)

install:
	@echo "Make sure you have the following packages installed:"
	@echo "  - libibverbs-dev"
	@echo "  - librdmacm-dev" 
	@echo "  - rdma-core"
	@echo ""
	@echo "On Ubuntu/Debian: sudo apt-get install libibverbs-dev librdmacm-dev rdma-core"
	@echo "On RHEL/CentOS: sudo yum install libibverbs-devel librdmacm-devel rdma-core"

test: all
	@echo "To test RDMA communication:"
	@echo "1. Start the server: ./rdma_server <port>"
	@echo "2. In another terminal, start the client: ./rdma_client <server_ip> <port>"
	@echo "Example: ./rdma_server 12345"
	@echo "         ./rdma_client 172.26.47.38 12345"
	@echo ""
	@echo "To test TCP communication:"
	@echo "1. Start the server: ./tcp_server <port>"
	@echo "2. In another terminal, start the client: ./tcp_client <server_ip> <port>"
	@echo "Example: ./tcp_server 12346"
	@echo "         ./tcp_client 127.0.0.1 12346"