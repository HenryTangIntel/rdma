#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

class RDMAServer {
private:
    struct rdma_cm_id *listen_id;
    struct rdma_cm_id *conn_id;
    struct rdma_event_channel *ec;
    struct ibv_pd *pd;
    struct ibv_comp_channel *comp_chan;
    struct ibv_cq *cq;
    struct ibv_mr *mr;
    char *buffer;
    static const size_t BUFFER_SIZE = 4096;
    static const int MAX_WR = 16;

public:
    RDMAServer() : listen_id(nullptr), conn_id(nullptr), ec(nullptr), 
                   pd(nullptr), comp_chan(nullptr), cq(nullptr), 
                   mr(nullptr), buffer(nullptr) {
        buffer = new char[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
    }

    ~RDMAServer() {
        cleanup();
        delete[] buffer;
    }

    int initialize(const std::string& port) {
        struct sockaddr_in addr;
        int ret;

        ec = rdma_create_event_channel();
        if (!ec) {
            std::cerr << "Failed to create event channel\n";
            return -1;
        }

        ret = rdma_create_id(ec, &listen_id, nullptr, RDMA_PS_TCP);
        if (ret) {
            std::cerr << "Failed to create listen ID\n";
            return ret;
        }

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(std::stoi(port));

        ret = rdma_bind_addr(listen_id, (struct sockaddr*)&addr);
        if (ret) {
            std::cerr << "Failed to bind address\n";
            return ret;
        }

        ret = rdma_listen(listen_id, 1);
        if (ret) {
            std::cerr << "Failed to listen\n";
            return ret;
        }

        std::cout << "RDMA server listening on port " << port << std::endl;
        return 0;
    }

    int waitForConnection() {
        struct rdma_cm_event *event;
        int ret;

        while (rdma_get_cm_event(ec, &event) == 0) {
            switch (event->event) {
                case RDMA_CM_EVENT_CONNECT_REQUEST:
                    conn_id = event->id;
                    ret = handleConnectRequest();
                    if (ret) {
                        rdma_ack_cm_event(event);
                        return ret;
                    }
                    break;

                case RDMA_CM_EVENT_ESTABLISHED:
                    std::cout << "Connection established\n";
                    rdma_ack_cm_event(event);
                    return 0; // Exit event loop after connection established

                case RDMA_CM_EVENT_DISCONNECTED:
                    std::cout << "Client disconnected\n";
                    rdma_ack_cm_event(event);
                    return 0;

                default:
                    break;
            }
            rdma_ack_cm_event(event);
        }
        return 0;
    }

    int sendMessage(const std::string& message) {
        if (!conn_id || !mr) {
            std::cerr << "Connection or memory region not ready\n";
            return -1;
        }

        strncpy(buffer, message.c_str(), BUFFER_SIZE - 1);
        buffer[BUFFER_SIZE - 1] = '\0';

        struct ibv_sge sge;
        struct ibv_send_wr send_wr, *bad_wr;

        memset(&sge, 0, sizeof(sge));
        sge.addr = (uintptr_t)buffer;
        sge.length = strlen(buffer) + 1;
        sge.lkey = mr->lkey;

        memset(&send_wr, 0, sizeof(send_wr));
        send_wr.wr_id = 1;
        send_wr.sg_list = &sge;
        send_wr.num_sge = 1;
        send_wr.opcode = IBV_WR_SEND;
        send_wr.send_flags = IBV_SEND_SIGNALED;

        int ret = ibv_post_send(conn_id->qp, &send_wr, &bad_wr);
        if (ret) {
            std::cerr << "Failed to post send\n";
            return ret;
        }

        std::cout << "Message sent: " << message << std::endl;
        return 0;
    }

    int receiveMessage() {
        if (!conn_id || !cq) {
            std::cerr << "Connection or completion queue not ready\n";
            return -1;
        }

        // Wait for completion of previously posted receive
        struct ibv_wc wc;
        do {
            while (ibv_poll_cq(cq, 1, &wc) == 0) {
                // Keep polling
            }
        } while (wc.wr_id != 2); // Wait for receive completion (wr_id = 2)

        if (wc.status != IBV_WC_SUCCESS) {
            std::cerr << "Work completion failed\n";
            return -1;
        }

        std::cout << "Message received: " << buffer << std::endl;
        return 0;
    }

private:
    int handleConnectRequest() {
        int ret;

        pd = ibv_alloc_pd(conn_id->verbs);
        if (!pd) {
            std::cerr << "Failed to allocate protection domain\n";
            return -1;
        }

        comp_chan = ibv_create_comp_channel(conn_id->verbs);
        if (!comp_chan) {
            std::cerr << "Failed to create completion channel\n";
            return -1;
        }

        cq = ibv_create_cq(conn_id->verbs, MAX_WR, nullptr, comp_chan, 0);
        if (!cq) {
            std::cerr << "Failed to create completion queue\n";
            return -1;
        }

        mr = ibv_reg_mr(pd, buffer, BUFFER_SIZE, 
                       IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ);
        if (!mr) {
            std::cerr << "Failed to register memory region\n";
            return -1;
        }

        struct ibv_qp_init_attr qp_attr;
        memset(&qp_attr, 0, sizeof(qp_attr));
        qp_attr.send_cq = cq;
        qp_attr.recv_cq = cq;
        qp_attr.qp_type = IBV_QPT_RC;
        qp_attr.cap.max_send_wr = MAX_WR;
        qp_attr.cap.max_recv_wr = MAX_WR;
        qp_attr.cap.max_send_sge = 1;
        qp_attr.cap.max_recv_sge = 1;

        ret = rdma_create_qp(conn_id, pd, &qp_attr);
        if (ret) {
            std::cerr << "Failed to create queue pair\n";
            return ret;
        }

        ret = rdma_accept(conn_id, nullptr);
        if (ret) {
            std::cerr << "Failed to accept connection\n";
            return ret;
        }

        // Post initial receive work request
        struct ibv_sge sge;
        struct ibv_recv_wr recv_wr, *bad_wr;

        memset(&sge, 0, sizeof(sge));
        sge.addr = (uintptr_t)buffer;
        sge.length = BUFFER_SIZE;
        sge.lkey = mr->lkey;

        memset(&recv_wr, 0, sizeof(recv_wr));
        recv_wr.wr_id = 2;
        recv_wr.sg_list = &sge;
        recv_wr.num_sge = 1;

        ret = ibv_post_recv(conn_id->qp, &recv_wr, &bad_wr);
        if (ret) {
            std::cerr << "Failed to post initial receive\n";
            return ret;
        }

        return 0;
    }

    void cleanup() {
        if (mr) ibv_dereg_mr(mr);
        if (cq) ibv_destroy_cq(cq);
        if (comp_chan) ibv_destroy_comp_channel(comp_chan);
        if (pd) ibv_dealloc_pd(pd);
        if (conn_id) rdma_destroy_id(conn_id);
        if (listen_id) rdma_destroy_id(listen_id);
        if (ec) rdma_destroy_event_channel(ec);
    }
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return 1;
    }

    RDMAServer server;
    
    int ret = server.initialize(argv[1]);
    if (ret) {
        std::cerr << "Failed to initialize server\n";
        return ret;
    }

    ret = server.waitForConnection();
    if (ret) {
        std::cerr << "Connection handling failed\n";
        return ret;
    }

    // Post receive first, then wait for client message
    std::cout << "Waiting for client message...\n";
    server.receiveMessage();
    
    // Send response back to client
    server.sendMessage("Hello from RDMA server!");

    return 0;
}