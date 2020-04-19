/*
 * Copyright (c) 2008 Princeton University
 * Copyright (c) 2016 Georgia Institute of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Niket Agarwal
 *          Tushar Krishna
 */


#ifndef __MEM_RUBY_NETWORK_GARNET2_0_NETWORKINTERFACE_HH__
#define __MEM_RUBY_NETWORK_GARNET2_0_NETWORKINTERFACE_HH__

#include <iostream>
#include <vector>

#include "mem/ruby/common/Consumer.hh"
#include "mem/ruby/network/garnet2.0/CommonTypes.hh"
#include "mem/ruby/network/garnet2.0/Credit.hh"
#include "mem/ruby/network/garnet2.0/CreditLink.hh"
#include "mem/ruby/network/garnet2.0/GarnetNetwork.hh"
#include "mem/ruby/network/garnet2.0/NetworkLink.hh"
#include "mem/ruby/network/garnet2.0/OutVcState.hh"
#include "mem/ruby/slicc_interface/Message.hh"
#include "params/GarnetNetworkInterface.hh"
#include "sim/sim_exit.hh"
#include <map>
#include <math.h> 
#include <fstream>
#include <chrono> 
#include <ctime>
#include <tuple>
#include "Sys.hh"
#include "CommonAPI.hh"
#define CLK_PERIOD 1
class MessageBuffer;
class flitBuffer;
class Sys;

class NetworkInterface : public ClockedObject, public Consumer,public CommonAPI
{
  public:
    typedef GarnetNetworkInterfaceParams Params;
    NetworkInterface(const Params *p);
    ~NetworkInterface();
	class InputPort;
	//mycode
    static std::map<int,std::map<int,int>> horizontal_allowed;
    static std::map<int,std::map<int,int>> vertical_allowed;
    int local_node_id;
    int right_node_id;
    int left_node_id;
    int up_node_id;
    int down_node_id;
    int Zpositive_node_id;
    int Znegative_node_id;
    int Fpositive_node_id;
    int Fnegative_node_id;
    int vnets_to_query;

    bool fired;
    int test;
    unsigned long long last_wake_up;
    int sim_comm_size(sim_comm comm, int* size);
    int sim_comm_get_rank(sim_comm comm, int *size);
    int sim_comm_set_rank(sim_comm comm, int rank);
    int sim_finish();
    double sim_time_resolution();
    int sim_init();
    timespec_t sim_get_time();
    void sim_schedule(timespec_t delta, void (*fun_ptr)(void *fun_arg), void *fun_arg);
    int sim_send(void *buffer, int count, int type, int dst, int tag, sim_request *request);
    int sim_recv(void *buffer, int count, int type, int src, int tag, sim_request *request, void (*msg_handler)(void *fun_arg), void* fun_arg);

    class Recv_Req{
        public:
            int count,type,src,tag,packets_to_receive;
            void (*msg_handler)(void *);
            void* fun_arg;
            Recv_Req(int count,int packets_to_receive,int type,int src,int tag,void (*msg_handler)(void *),void *fun_arg){
                this->packets_to_receive=packets_to_receive;this->count=count;this->type=type;this->src=src;this->tag=tag;this->msg_handler=msg_handler;this->fun_arg=fun_arg;
            }
    };
    class Send_Req{
        public:
            int count,type,dst,tag,packets_to_send,packet_size;
            void (*msg_handler)(void *);
            void* fun_arg;
            Send_Req(int count,int packets_to_send,int type,int dst,int tag,int packet_size){
                this->packets_to_send=packets_to_send;this->count=count;this->type=type;this->dst=dst;this->tag=tag;this->packet_size=packet_size;
            }
    };
    MsgPtr create_packet(int packet_size,int type,int dst,int tag,bool is_end,int vnet);
    std::map<unsigned long long,std::list<std::pair<void (*)(void *),void*>>> events_list;
    void call_events();
    std::map<int,std::list<Send_Req>> send_reqs;
    std::map<int,std::list<Recv_Req>> recv_reqs;
    int flit_width;
    MsgPtr template_msg;
    static void marker(void *argument){return;};
    int nextPowerOf2(int n);


    std::map<int,std::vector<std::pair<flit *,InputPort *>>> waiting_packets;
    std::map<int,int> waiting_packets_done;
    int *reserved_VCs;

    //for seperating logical vnet and physical vnet
    //std::vector<std::list<MsgPtr>> msg_mem;
    //int vnet_search_iterator;
    //MsgPtr get_packet(int physical_vnet);
    //void memorize_packet(MsgPtr packet,int physical_vnet);
    bool remove_packet_from_waiting_list(int vnet){
        return true;
    }

    
    void init();

    void addInPort(NetworkLink *in_link, CreditLink *credit_link);
    void addOutPort(NetworkLink *out_link, CreditLink *credit_link,
        SwitchID router_id);

    void dequeueCallback();
    void wakeup();
    void addNode(std::vector<MessageBuffer *> &inNode,
                 std::vector<MessageBuffer *> &outNode);

    void print(std::ostream& out) const;
    int get_vnet(int vc);
    void init_net_ptr(GarnetNetwork *net_ptr) { m_net_ptr = net_ptr; }

    uint32_t functionalWrite(Packet *);

    void scheduleFlit(flit *t_flit);
    bool testFreeVC(int vnet);
    int get_router_id(int vnet)
    {
        OutputPort *oPort = getOutportForVnet(vnet);
        assert(oPort);
        return oPort->routerID();
    }
    
    bool acquireFreeVC(int vnet);
    class OutputPort
    {
        public:
            OutputPort(NetworkLink *outLink, CreditLink *creditLink,
                int routerID)
            {
                _vnets = outLink->mVnets;
                _outFlitQueue = new flitBuffer();

                _outNetLink = outLink;
                _inCreditLink = creditLink;

                _routerID = routerID;
                _bitWidth = outLink->bitWidth;
            }

            flitBuffer *
            outFlitQueue()
            {
                return _outFlitQueue;
            }

            NetworkLink *
            outNetLink()
            {
                return _outNetLink;
            }

            CreditLink *
            inCreditLink()
            {
                return _inCreditLink;
            }

            int
            routerID()
            {
                return _routerID;
            }

            uint32_t bitWidth()
            {
                return _bitWidth;
            }

            bool isVnetSupported(int pVnet)
            {
                for (auto &it : _vnets) {
                    if ((it == -1) || (it == pVnet)) {
                        return true;
                    }
                }
                return false;

            }

            std::string
            printVnets()
            {
                std::stringstream ss;
                for (auto &it : _vnets) {
                    ss << it;
                    ss << " ";
                }
                return ss.str();
            }

        private:
            std::vector<int> _vnets;
            flitBuffer *_outFlitQueue;

            NetworkLink *_outNetLink;
            CreditLink *_inCreditLink;

            int _routerID;
            uint32_t _bitWidth;
    };

    class InputPort
    {
        public:
            InputPort(NetworkLink *inLink, CreditLink *creditLink)
            {
                _vnets = inLink->mVnets;
                _outCreditQueue = new flitBuffer();

                _inNetLink = inLink;
                _outCreditLink = creditLink;
                _bitWidth = inLink->bitWidth;
            }

            flitBuffer *
            outCreditQueue()
            {
                return _outCreditQueue;
            }

            NetworkLink *
            inNetLink()
            {
                return _inNetLink;
            }

            CreditLink *
            outCreditLink()
            {
                return _outCreditLink;
            }

            bool isVnetSupported(int pVnet)
            {
                for (auto &it : _vnets) {
                    if ((it == -1) || (it == pVnet)) {
                        return true;
                    }
                }
                return false;

            }

            void sendCredit(Credit *cFlit)
            {
                _outCreditQueue->insert(cFlit);
            }

            uint32_t bitWidth()
            {
                return _bitWidth;
            }

            std::string
            printVnets()
            {
                std::stringstream ss;
                for (auto &it : _vnets) {
                    ss << it;
                    ss << " ";
                }
                return ss.str();
            }

        private:
            std::vector<int> _vnets;
            flitBuffer *_outCreditQueue;

            NetworkLink *_inNetLink;
            CreditLink *_outCreditLink;
            uint32_t _bitWidth;
    };
  private:
    GarnetNetwork *m_net_ptr;
    const NodeID m_id;
    const int m_virtual_networks, m_vc_per_vnet, m_num_vcs;
    std::vector<OutVcState *> m_out_vc_state;
    std::vector<int> m_vc_allocator;
    int m_vc_round_robin; // For round robin scheduling
    int *m_vc_round_robin_per_vnet; // For round robin scheduling per vnet
    std::vector<OutputPort *> outPorts;
    std::vector<InputPort *> inPorts;
    int m_deadlock_threshold;

    // Queue for stalled flits
    std::deque<flit *> m_stall_queue;
    std::vector<int> m_stall_count;

    // Input Flit Buffers
    // The flit buffers which will serve the Consumer
    std::vector<flitBuffer *>  m_ni_out_vcs;
    std::vector<Tick> m_ni_out_vcs_enqueue_time;

    // The Message buffers that takes messages from the protocol
    std::vector<MessageBuffer *> inNode_ptr;
    // The Message buffers that provides messages to the protocol
    std::vector<MessageBuffer *> outNode_ptr;
    // When a vc stays busy for a long time, it indicates a deadlock
    std::vector<int> vc_busy_counter;

    bool checkStallQueue();
    bool flitisizeMessage(MsgPtr msg_ptr, int vnet);
    bool flitisizeMessage(int vc,int packet_size,int type,int dst,int tag,bool is_end,int vnet);
    int calculateVC(int vnet);

    void scheduleOutputLink();
    void scheduleOutputLinkForVnet(int vn);
    void checkReschedule();

    void incrementStats(flit *t_flit);

    InputPort *getInportForVnet(int vnet);
    OutputPort *getOutportForVnet(int vnet);
    //mycode
    Sys *my_generator;
    int template_message_received;
};

#endif // __MEM_RUBY_NETWORK_GARNET2_0_NETWORKINTERFACE_HH__
