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


#include "mem/ruby/network/garnet2.0/NetworkInterface.hh"

#include <cassert>
#include <cmath>

#include "base/cast.hh"
#include "base/stl_helpers.hh"
#include "debug/RubyNetwork.hh"
#include "mem/ruby/network/MessageBuffer.hh"
#include "mem/ruby/network/garnet2.0/Credit.hh"
#include "mem/ruby/network/garnet2.0/flitBuffer.hh"
#include "mem/ruby/slicc_interface/Message.hh"
#include <iostream>

using namespace std;
using m5::stl_helpers::deletePointers;


std::map<int,std::map<int,int>> NetworkInterface::horizontal_allowed;
std::map<int,std::map<int,int>> NetworkInterface::vertical_allowed;

NetworkInterface::NetworkInterface(const Params *p)
    : ClockedObject(p), Consumer(this), m_id(p->id),
      m_virtual_networks(p->virt_nets), m_vc_per_vnet(p->vcs_per_vnet),
      m_num_vcs(m_vc_per_vnet * m_virtual_networks),
      m_deadlock_threshold(p->garnet_deadlock_threshold),
      vc_busy_counter(m_virtual_networks, 0)
{
    m_vc_round_robin = 0;
    m_ni_out_vcs.resize(m_num_vcs);
    m_ni_out_vcs_enqueue_time.resize(m_num_vcs);
    
    //mycode
    m_vc_round_robin_per_vnet=new int[m_virtual_networks];
    for(int i=0;i<m_virtual_networks;i++){
        m_vc_round_robin_per_vnet[i]=0;
    }

    // instantiating the NI flit buffers
    for (int i = 0; i < m_num_vcs; i++) {
        m_ni_out_vcs[i] = new flitBuffer();
        m_ni_out_vcs_enqueue_time[i] = Tick(INFINITE_);
    }

    m_vc_allocator.resize(m_virtual_networks); // 1 allocator per vnet
    reserved_VCs=new int[m_virtual_networks];
    for (int i = 0; i < m_virtual_networks; i++) {
        m_vc_allocator[i] = 0;
        reserved_VCs[i]=0;
    }

    m_stall_count.resize(m_virtual_networks);
    enabled=true;
    fired=false;
    last_wake_up=0;
    test=0;
	
	//cout << "NI created"<<endl;
}

void
NetworkInterface::init()
{
	//mycode
    //cout<<"my id: "<<m_id<<endl;
    if(m_id<m_net_ptr->getNumRouters()){
        map<int,int> temp;
        map<int,int> temp2;
        horizontal_allowed[m_id]=temp;
        vertical_allowed[m_id]=temp2;
        flit_width=m_net_ptr->get_flit_width();

        //int package_rows=m_net_ptr->get_package_rows();//m_net_ptr->getNumRouters()
        int local_cols=m_net_ptr->get_num_cpus()/m_net_ptr->get_num_packages();
        int package_cols=-1;
        
        if(m_net_ptr->get_topology()=="Torus3D"){
            package_cols=m_net_ptr->get_num_packages()/m_net_ptr->get_package_rows();
        }
        else if(m_net_ptr->get_topology()=="Torus4D"){
            package_cols=m_net_ptr->get_package_cols();
        }
        else if(m_net_ptr->get_topology()=="Torus5D"){
            package_cols=m_net_ptr->get_package_cols();
        }
        else if(m_net_ptr->get_topology()=="AllToAll"){
            package_cols=1;
        }
        else if(m_net_ptr->get_topology()=="NV_Switch"){
            package_cols=1;
        }
        
        if((m_id+1)%local_cols!=0){
            local_node_id=m_id+1;
        }
        else{
            local_node_id=m_id-(m_id%local_cols);
        }
        assert(package_cols>0);
        right_node_id=(m_id-(m_id%(local_cols*package_cols))) +(m_id+local_cols)%(local_cols*package_cols);
        left_node_id=(m_id-(m_id%(local_cols*package_cols)))+(m_id-local_cols)%(local_cols*package_cols);
        if(m_net_ptr->get_topology()=="Torus3D"){
            up_node_id=(m_id+(local_cols*package_cols))%m_net_ptr->getNumRouters();
            down_node_id=(m_id-(local_cols*package_cols))%m_net_ptr->getNumRouters();
        }
        else if(m_net_ptr->get_topology()=="Torus4D"){
            up_node_id=(m_id-(m_id%(local_cols*package_cols*m_net_ptr->get_package_rows())))+((m_id+(local_cols*package_cols))%(local_cols*package_cols*m_net_ptr->get_package_rows()));
            down_node_id=(m_id-(m_id%(local_cols*package_cols*m_net_ptr->get_package_rows())))+((m_id-(local_cols*package_cols))%(local_cols*package_cols*m_net_ptr->get_package_rows()));
            Zpositive_node_id=(m_id+(local_cols*package_cols*m_net_ptr->get_package_rows()))%m_net_ptr->getNumRouters();
            Znegative_node_id=(m_id-(local_cols*package_cols*m_net_ptr->get_package_rows()))%m_net_ptr->getNumRouters();
        }
        else if(m_net_ptr->get_topology()=="Torus5D"){
            up_node_id=(m_id-(m_id%(local_cols*package_cols*m_net_ptr->get_package_rows())))+
            ((m_id+(local_cols*package_cols))%(local_cols*package_cols*m_net_ptr->get_package_rows()));
            down_node_id=(m_id-(m_id%(local_cols*package_cols*m_net_ptr->get_package_rows())))+
            ((m_id-(local_cols*package_cols))%(local_cols*package_cols*m_net_ptr->get_package_rows()));
            
            Zpositive_node_id=(m_id-(m_id%(local_cols*package_cols*m_net_ptr->get_package_rows()*m_net_ptr->get_package_height())))+
            ((m_id+(local_cols*package_cols*m_net_ptr->get_package_rows()))%(local_cols*package_cols*m_net_ptr->get_package_rows()*m_net_ptr->get_package_height()));
            Znegative_node_id=(m_id-(m_id%(local_cols*package_cols*m_net_ptr->get_package_rows()*m_net_ptr->get_package_height())))+
            ((m_id-(local_cols*package_cols*m_net_ptr->get_package_rows()))%(local_cols*package_cols*m_net_ptr->get_package_rows()*m_net_ptr->get_package_height()));
            
            Fpositive_node_id=(m_id+(local_cols*package_cols*m_net_ptr->get_package_rows()*m_net_ptr->get_package_height()))%m_net_ptr->getNumRouters();
            Fnegative_node_id=(m_id-(local_cols*package_cols*m_net_ptr->get_package_rows()*m_net_ptr->get_package_height()))%m_net_ptr->getNumRouters();
        }
    }
    
	//allowed[m_id]=m_net_ptr->get_local_parallel_reduce();
	
    for (int i = 0; i < m_num_vcs; i++) {
        m_out_vc_state.push_back(new OutVcState(i, m_net_ptr));
    }
    //mycode
    template_message_received=0;
    
    for (int i = 0; i < m_virtual_networks; i++) {
        std::vector<std::pair<flit *,InputPort *>> tempTupleVector;
        waiting_packets[i]=tempTupleVector;
        waiting_packets_done[i]=0;

        std::list<Recv_Req> empty_r;
        recv_reqs[i]=empty_r;

        std::list<Send_Req> empty_s;
        send_reqs[i]=empty_s;

    }
    //cout<<"my inports: "<<inPorts.size()<<endl;
    //cout<<"output buffer size: "<<m_out_vc_state.size()<<endl;
    
    
}

NetworkInterface::~NetworkInterface()
{
	
    deletePointers(m_out_vc_state);
    deletePointers(m_ni_out_vcs);
}

void
NetworkInterface::addInPort(NetworkLink *in_link,
                              CreditLink *credit_link)
{
    InputPort *newInPort = new InputPort(in_link, credit_link);
    inPorts.push_back(newInPort);
    DPRINTF(RubyNetwork, "Adding input port:%s with vnets %s\n",
    in_link->name(), newInPort->printVnets());

    in_link->setLinkConsumer(this);
    credit_link->setSourceQueue(newInPort->outCreditQueue(), this);

}

void
NetworkInterface::addOutPort(NetworkLink *out_link,
                             CreditLink *credit_link,
                             SwitchID router_id)
{
    OutputPort *newOutPort = new OutputPort(out_link, credit_link, router_id);
    outPorts.push_back(newOutPort);

    DPRINTF(RubyNetwork, "OutputPort:%s Vnet: %s\n",
    out_link->name(), newOutPort->printVnets());

    out_link->setSourceQueue(newOutPort->outFlitQueue(), this);
    credit_link->setLinkConsumer(this);
}

void
NetworkInterface::addNode(vector<MessageBuffer *>& in,
                            vector<MessageBuffer *>& out)
{
    inNode_ptr = in;
    outNode_ptr = out;

    for (auto& it : in) {
        if (it != nullptr) {
            it->setConsumer(this);
        }
    }
}

void
NetworkInterface::dequeueCallback()
{
    // An output MessageBuffer has dequeued something this cycle and there
    // is now space to enqueue a stalled message. However, we cannot wake
    // on the same cycle as the dequeue. Schedule a wake at the soonest
    // possible time (next cycle).
    scheduleEventAbsolute(clockEdge(Cycles(1)));
}

void
NetworkInterface::incrementStats(flit *t_flit)
{
    int vnet = t_flit->get_vnet();

    // Latency
    m_net_ptr->increment_received_flits(vnet);
    Tick network_delay =
        t_flit->get_dequeue_time() -
        t_flit->get_enqueue_time() - cyclesToTicks(Cycles(1));
    Tick src_queueing_delay = t_flit->get_src_delay();
    Tick dest_queueing_delay = (curTick() - t_flit->get_dequeue_time());
    Tick queueing_delay = src_queueing_delay + dest_queueing_delay;

    m_net_ptr->increment_flit_network_latency(network_delay, vnet);
    m_net_ptr->increment_flit_queueing_latency(queueing_delay, vnet);

    if (t_flit->get_type() == TAIL_ || t_flit->get_type() == HEAD_TAIL_) {
        m_net_ptr->increment_received_packets(vnet);
        m_net_ptr->increment_packet_network_latency(network_delay, vnet);
        m_net_ptr->increment_packet_queueing_latency(queueing_delay, vnet);
    }

    // Hops
    m_net_ptr->increment_total_hops(t_flit->get_route().hops_traversed);
}

/*
 * The NI wakeup checks whether there are any ready messages in the protocol
 * buffer. If yes, it picks that up, flitisizes it into a number of flits and
 * puts it into an output buffer and schedules the output link. On a wakeup
 * it also checks whether there are flits in the input link. If yes, it picks
 * them up and if the flit is a tail, the NI inserts the corresponding message
 * into the protocol buffer. It also checks for credits being sent by the
 * downstream router.
 */

void
NetworkInterface::wakeup()
{
    if(last_wake_up==curTick()){
        return;
    }
    if(m_id==7){
        //std::cout<<"Hey! I am node 7 and I have ben waken upt at tick: "<<curTick()<<std::endl;
    }
    last_wake_up=curTick();
    call_events();
    //cout<<"number of Vnets: "<<outNode_ptr.size()<<endl;
	//if(m_net_ptr->get_router_id(m_id, 0)<m_net_ptr->get_num_router())
	//cout << "The allowed of Router: "<< m_net_ptr->get_router_id(m_id, 0) << " is : "<< allowed[m_net_ptr->get_router_id(m_id, 0)] << endl;
    std::ostringstream oss;
    for (auto &oPort: outPorts) {
        oss << oPort->routerID() << "[" << oPort->printVnets() << "] ";
    }
    DPRINTF(RubyNetwork, "Network Interface %d connected to router:%s "
            "woke up. Period: %ld\n", m_id, oss.str(), clockPeriod());

    assert(curTick() == clockEdge());
    MsgPtr msg_ptr;
    Tick curTime = clockEdge();

    
	if(template_message_received==0){
        for (int vnet = 0; vnet < inNode_ptr.size(); ++vnet) {
            MessageBuffer *b = inNode_ptr[vnet%3];
            if (b == nullptr) {
                continue;
            }        
            if (b->isReady(curTime)) { // Is there a message waiting
                template_message_received=1;
                msg_ptr = b->peekMsgPtr();
                template_msg=msg_ptr;
                std::vector<int> empty;
                std::cout<<"instantiated"<<std::endl;
                int horiz_queues=m_net_ptr->horizontal_vnets1.size()*2;
                int ver_queues=m_net_ptr->vertical_vnets1.size()*2;
                if(m_net_ptr->get_topology()=="AllToAll" || m_net_ptr->get_topology()=="NV_Switch"){
                    horiz_queues=m_net_ptr->links_per_tile;
                    ver_queues=0;
                }
                my_generator=new Sys(this,m_id,m_net_ptr->num_passes,
                        m_net_ptr->get_num_cpus()/m_net_ptr->get_num_packages(),m_net_ptr->get_package_rows(),
                        m_net_ptr->get_num_packages()/m_net_ptr->get_package_rows(),
                        1,1,m_net_ptr->local_vnets.size(),ver_queues,horiz_queues,
                        0,0,m_net_ptr->sys_input,m_net_ptr->get_workload(),m_net_ptr->get_comm_scale(),
                        m_net_ptr->get_compute_scale(),m_net_ptr->total_stat_rows,
                        m_net_ptr->stat_row,m_net_ptr->path,m_net_ptr->run_name);

                b->dequeue(clockEdge());
                scheduleEvent(Cycles(10));
                scheduleOutputLink();
                return;
            }
        }
    }
    if(fired==false && template_message_received==1){
        std::cout<<"fired"<<std::endl;
        fired=true;
        my_generator->workload->fire();
        return;
    }
    
	// Checking for messages coming from the protocol
    // can pick up a message/cycle for each virtual net
    if(template_message_received==1){
        //my_generator->iterate();
        if(enabled) {
            for (auto &vn:send_reqs) {
                int vnet = vn.first;//0; vnet < vnets_to_query; ++vnet
                if (vn.second.size()>0) { // Is there a message waiting
                    Send_Req &sr=vn.second.front();
                    /*MsgPtr new_msg_ptr;
                    Message *new_net_msg_ptr;
                    if(m_id!=1){
                        b->dequeue(curTime);
                        goto SCHEDULE;
                    }
                    else{
                        my_generator=new Sys(m_net_ptr->local_vnets,m_net_ptr->vertical_vnets1,m_net_ptr->vertical_vnets2,m_net_ptr->horizontal_vnets1,
                                     m_net_ptr->horizontal_vnets2,local_node_id,right_node_id,left_node_id,up_node_id,down_node_id,
                                     m_net_ptr->get_local_parallel_reduce(),m_net_ptr->get_vertical_parallel_reduce(),m_net_ptr->get_horizontal_parallel_reduce(),
                                     m_net_ptr->get_local_burst_length(),m_net_ptr->get_horizontal_burst_length(),
                                     m_net_ptr->get_vertical_burst_length(),m_net_ptr->get_local_message_size(),m_net_ptr->get_horizontal_message_size(),
                                     m_net_ptr->get_vertical_message_size(),m_net_ptr->get_burst_interval(),m_net_ptr->get_flit_width(),
                                     m_net_ptr->get_processing_delay(),m_net_ptr->get_chunk_numbers(),msg_ptr);
                        cout<<"packet is going to be injected from node: "<<m_id<<endl;
                         new_msg_ptr = msg_ptr->clone();
                         new_net_msg_ptr = new_msg_ptr.get();
                         NodeID destID=25;
                         NetDest personal_dest;
                         for (int m = 0; m < (int) MachineType_NUM; m++) {
                            if ((destID >= MachineType_base_number((MachineType) m)) &&
                                destID < MachineType_base_number((MachineType) (m+1))) {
                                // calculating the NetDest associated with this destID
                                personal_dest.clear();
                                personal_dest.add((MachineID) {(MachineType) m, (destID -
                                    MachineType_base_number((MachineType) m))});
                                new_net_msg_ptr->getDestination() = personal_dest;
                                break;
                            }
                        }
                    }*/
                    int vc_num=calculateVC(vnet);
                    if (vc_num!=-1) { //vnet
                        //cout<<"message found to send at node: "<<m_id<<" , at vnet: "<<vnet<<" ,messages left: "<<vn.second.size()<<"flits: "<<
                        //msg_ptr.get()->getFlits()<<endl;
                        //MsgPtr msg_ptr=create_packet(sr.packet_size,sr.type,sr.dst,sr.tag,true,vnet);
                        flitisizeMessage(vc_num,sr.packet_size,sr.type,sr.dst,sr.tag,true,vnet);
                        //flitisizeMessage(msg_ptr, vnet, msg_ptr.get()->getFlits(),vc_num);
                        sr.packets_to_send--;
                        if(sr.packets_to_send==0){
                            vn.second.pop_front();
                        }
                    }
                }
            }
        }
    }

    if(enabled) {
        scheduleOutputLink();
    }

    // Check if there are flits stalling a virtual channel. Track if a
    // message is enqueued to restrict ejection to one message per cycle.
    //bool messageEnqueuedThisCycle = checkStallQueue();
    //checkStallQueue();
    if(enabled) {
        /*********** Check the incoming flit link **********/
        DPRINTF(RubyNetwork, "Number of iPorts: %d\n", inPorts.size());
        for (auto &iPort: inPorts) {
            NetworkLink *inNetLink = iPort->inNetLink();
            DPRINTF(RubyNetwork, "Checking input port:%s with vnets %s\n",
                    inNetLink->name(), iPort->printVnets());
            if (inNetLink->isReady(curTick())) {
                flit *t_flit = inNetLink->consumeLink();
                DPRINTF(RubyNetwork, "Recieved flit:%s\n", *t_flit);
                assert(t_flit->m_width == iPort->bitWidth());

                int vnet = t_flit->get_msg_ptr()->vnet_to_fetch;
                //cout<<"my received vnet: "<<vnet<<endl;

                t_flit->set_dequeue_time(curTick());

                if(m_id==0){
                    //std::cout<<"I am node 0 and I have recieved "<<test++<<" flits so far"<<std::endl;
                }
                // If a tail flit is received, enqueue into the protocol buffers
                // if space is available. Otherwise, exchange non-tail flits for
                // credits.
                if (t_flit->get_type() == TAIL_ ||
                    t_flit->get_type() == HEAD_TAIL_) {
                    //cout<<"message received at node: "<<m_id<<" , at vnet: "<<vnet<<endl;
                    //std::list<Recv_Req> &reqs=recv_reqs[vnet];
                    std::list<Recv_Req>::iterator it=recv_reqs[vnet].begin();
                    MsgPtr mptr=t_flit->get_msg_ptr();
                    bool found= false;
                    while(it!=recv_reqs[vnet].end()){
                        //std::cout<<"caution, it src: "<<(*it).src<<" and flit src: "<<t_flit->get_msg_ptr().get()->capi_src<<
                        //" and it tag: "<<(*it).tag<<" and flit tag: "<<t_flit->get_msg_ptr().get()->capi_tag<<std::endl;
                        assert((*it).packets_to_receive>0);
                        if((*it).type==t_flit->get_msg_ptr().get()->capi_type &&
                        (*it).src==t_flit->get_msg_ptr().get()->capi_src &&
                        (*it).tag==t_flit->get_msg_ptr().get()->capi_tag){
                            (*it).packets_to_receive--;
                            if(((*it).packets_to_receive)==0){
                                (*((*it).msg_handler))((*it).fun_arg);
                                recv_reqs[vnet].erase(it);
                                //cout<<"received message matched at node "<<m_id<<" , at vnet: "<<vnet<<endl;
                            }
                            found= true;
                            break;
                        } //count,type,src,tag;
                        std::advance(it,1);
                    }
                    assert(found==true);
                    Credit *cFlit = new Credit(t_flit->get_vc(),
                                               true, curTick());
                    assert(cFlit != NULL);
                    iPort->sendCredit(cFlit);
                    // Update stats and delete flit pointer
                    incrementStats(t_flit);
                    //std::cout<<"delete tflit at time: "<<curTick()<<" for node: "<<m_id<<std::endl;
                    delete t_flit;
                    //mptr.reset();
                     /*else {
                    // No space available- Place tail flit in stall queue and
                    // set up a callback for when protocol buffer is dequeued.
                    // Stat update and flit pointer deletion will occur upon
                    // unstall.
                    m_stall_queue.push_back(t_flit);
                    m_stall_count[vnet]++;
                    //mycode
                    //vertical_allowed[myid][t_flit->get_msg_ptr().get()->getStreamNum()]++;
                    
                    auto cb = std::bind(&NetworkInterface::dequeueCallback,
                                           this);
                    outNode_ptr[vnet%3]->registerDequeueCallback(cb);
                }*/
                } else {
                    // Non-tail flit. Send back a credit but not VC free signal.
                    Credit *cFlit = new Credit(t_flit->get_vc(), false,
                                               curTick());
                    if (my_generator->id == 0 && vnet == 5) {
                        //cout<<"non tail flit"<<endl;
                        // Simply send a credit back since we are not buffering
                        // this flit in the NI
                    }
                    iPort->sendCredit(cFlit);

                    // Update stats and delete flit pointer.
                    incrementStats(t_flit);
                    delete t_flit;
                }
            }
        }
    }

    /****************** Check the incoming credit link *******/
    if(enabled){
        for (auto &oPort: outPorts) {
            CreditLink *inCreditLink = oPort->inCreditLink();
            DPRINTF(RubyNetwork, "Checking input port:%s with vnets %s\n",
                inCreditLink->name(), oPort->printVnets());
            if (inCreditLink->isReady(curTick())) {
                Credit *t_credit = (Credit*) inCreditLink->consumeLink();
                m_out_vc_state[t_credit->get_vc()]->increment_credit();
                if (t_credit->is_free_signal()) {
                    m_out_vc_state[t_credit->get_vc()]->setState(IDLE_,
                        curTick());
                }
                delete t_credit;
            }
        }


        // It is possible to enqueue multiple outgoing credit flits if a message
        // was unstalled in the same cycle as a new message arrives. In this
        // case, we should schedule another wakeup to ensure the credit is sent
        // back.
        for (auto &iPort: inPorts) {
            if (iPort->outCreditQueue()->getSize() > 0) {
                DPRINTF(RubyNetwork, "Sending a credit via %s at %ld\n",
                iPort->outCreditLink()->name(), clockEdge(Cycles(1)));
                iPort->outCreditLink()->
                    scheduleEventAbsolute(clockEdge(Cycles(1)));
            }
        }
    }
    checkReschedule();
}
bool
NetworkInterface::checkStallQueue()
{
    bool messageEnqueuedThisCycle = false;
    Tick curTime = clockEdge();

    if (!m_stall_queue.empty()) {
        for (auto stallIter = m_stall_queue.begin();
             stallIter != m_stall_queue.end(); ) {
            flit *stallFlit = *stallIter;
            int vnet = stallFlit->get_vnet();

            // If we can now eject to the protocol buffer, send back credits
            if (outNode_ptr[vnet%3]->areNSlotsAvailable(1, curTime)) {
                outNode_ptr[vnet%3]->enqueue(stallFlit->get_msg_ptr(), curTime,
                                           cyclesToTicks(Cycles(1)));

                // Send back a credit with free signal now that the VC is no
                // longer stalled.
                Credit *cFlit = new Credit(stallFlit->get_vc(), true,
                                               curTick());
                InputPort *iPort = getInportForVnet(vnet);
                assert(iPort);

                iPort->sendCredit(cFlit);

                // Update Stats
                incrementStats(stallFlit);

                // Flit can now safely be deleted and removed from stall
                // queue
                delete stallFlit;
                m_stall_queue.erase(stallIter);
                m_stall_count[vnet]--;

                // If there are no more stalled messages for this vnet, the
                // callback on it's MessageBuffer is not needed.
                if (m_stall_count[vnet] == 0)
                    outNode_ptr[vnet%3]->unregisterDequeueCallback();

                messageEnqueuedThisCycle = true;
                break;
            } else {
                ++stallIter;
            }
        }
    }

    return messageEnqueuedThisCycle;
}

bool
NetworkInterface::flitisizeMessage(int vc,int packet_size,int type,int dst,int tag,bool is_end,int vnet)
{
    /*if(my_generator->method=="proposed" || (my_generator->method=="baseline" && reserved_VCs[vnet]>0)){
        reserved_VCs[vnet]--;
        if(reserved_VCs[vnet]<0){
            std::cout<<"troubled vnet: "<<vnet<<" ,for gen id: "<<my_generator->id<<std::endl;
            for(int i=0;i<m_virtual_networks;i++){
                std::cout<<"reserved vc: "<<reserved_VCs[i]<<std::endl;
            }
        }
        assert(reserved_VCs[vnet]>=0);
    }*/
    MsgPtr msg_ptr=create_packet(packet_size,type,dst,tag,is_end,vnet);
    Message *net_msg_ptr = msg_ptr.get();
    NetDest net_msg_dest = net_msg_ptr->getDestination();

    // gets all the destinations associated with this message.
    vector<NodeID> dest_nodes = net_msg_dest.getAllDest();

    // Number of flits is dependent on the link bandwidth available.
    // This is expressed in terms of bytes/cycle or the flit size
    
    //mycode
    //OutputPort *oPortTest = getOutportForVnet(5);
    //assert(oPortTest);
    
    OutputPort *oPort = getOutportForVnet(vnet);
    if(!oPort){
        std::cout<<"vnet is: "<<vnet<<std::endl;
    }
    assert(oPort);
    //int num_flits=(int)(flitNum/((float)m_net_ptr->get_package_link_efficiency()));
    int num_flits = (int) ceil(((double) packet_size*8)/oPort->bitWidth());
    //std::cout<<"num flit is: "<<num_flits<<std::endl;

    if(my_generator->id==0 && vnet==5){
        //std::cout<<"flits after filitisize: "<<num_flits<<" ,time: "<<curTick()<<std::endl;
    }
    //std::cout<<"Message size in bytes: "<<m_net_ptr->MessageSizeType_to_int(net_msg_ptr->getMessageSize())<<std::endl;
    /*int num_flits = (int) ceil((double) m_net_ptr->MessageSizeType_to_int(
        net_msg_ptr->getMessageSize())*8/oPort->bitWidth());
    DPRINTF(RubyNetwork, "Message Size:%d vnet:%d bitWidth:%d\n",
    m_net_ptr->MessageSizeType_to_int(net_msg_ptr->getMessageSize()),
    vnet, oPort->bitWidth());*/
    
    //if(vnet==4){
        //cout<<" flit num for VC4: "<<num_flits<<endl;
        //num_flits=4;
    //}
    
    // loop to convert all multicast messages into unicast messages
    for (int ctr = 0; ctr < dest_nodes.size(); ctr++) {

        // this will return a free output virtual channel
        //int vc = calculateVC(vnet);

        if (vc == -1) {
            return false ;
        }
        MsgPtr new_msg_ptr = msg_ptr;
        NodeID destID = dest_nodes[ctr];
        //cout<<"Node ID: "<< destID<<endl;
        
        Message *new_net_msg_ptr = new_msg_ptr.get();
        if (dest_nodes.size() > 1) {
            NetDest personal_dest;
            for (int m = 0; m < (int) MachineType_NUM; m++) {
                if ((destID >= MachineType_base_number((MachineType) m)) &&
                    destID < MachineType_base_number((MachineType) (m+1))) {
                    // calculating the NetDest associated with this destID
                    personal_dest.clear();
                    personal_dest.add((MachineID) {(MachineType) m, (destID -
                        MachineType_base_number((MachineType) m))});
                    new_net_msg_ptr->getDestination() = personal_dest;
                    break;
                }
            }
            net_msg_dest.removeNetDest(personal_dest);
            // removing the destination from the original message to reflect
            // that a message with this particular destination has been
            // flitisized and an output vc is acquired
            net_msg_ptr->getDestination().removeNetDest(personal_dest);
        }
        //new_net_msg_ptr->logical_vnet=logical_vnet;
        // Embed Route into the flits
        // NetDest format is used by the routing table
        // Custom routing algorithms just need destID

        RouteInfo route;
        route.vnet = vnet;
        route.net_dest = new_net_msg_ptr->getDestination();
        route.src_ni = m_id;
        route.src_router = oPort->routerID();
        route.dest_ni = destID;
        route.dest_router = m_net_ptr->get_router_id(destID, vnet);
        //cout<<"dest route: "<<route.dest_router<<endl;

        // initialize hops_traversed to -1
        // so that the first router increments it to 0
        route.hops_traversed = -1;

        m_net_ptr->increment_injected_packets(vnet);
        for (int i = 0; i < num_flits; i++) {
            m_net_ptr->increment_injected_flits(vnet);
            flit *fl = new flit(i, vc, vnet, route, num_flits, new_msg_ptr,
                    packet_size,  //m_net_ptr->MessageSizeType_to_int(net_msg_ptr->getMessageSize())
                oPort->bitWidth(), curTick());

            fl->set_src_delay(curTick() - (msg_ptr->getTime()));
            m_ni_out_vcs[vc]->insert(fl);
        }

        m_ni_out_vcs_enqueue_time[vc] = curTick();
        m_out_vc_state[vc]->setState(ACTIVE_, curTick());
    }
    return true ;
}

// Embed the protocol message into flits
bool
NetworkInterface::flitisizeMessage(MsgPtr msg_ptr, int vnet)
{
    Message *net_msg_ptr = msg_ptr.get();
    NetDest net_msg_dest = net_msg_ptr->getDestination();

    // gets all the destinations associated with this message.
    vector<NodeID> dest_nodes = net_msg_dest.getAllDest();

    // Number of flits is dependent on the link bandwidth available.
    // This is expressed in terms of bytes/cycle or the flit size
    
    //mycode
    //OutputPort *oPortTest = getOutportForVnet(5);
    //assert(oPortTest);
    
    OutputPort *oPort = getOutportForVnet(vnet);
    assert(oPort);
    int num_flits = (int) ceil((double) m_net_ptr->MessageSizeType_to_int(
        net_msg_ptr->getMessageSize())*8/oPort->bitWidth());
    DPRINTF(RubyNetwork, "Message Size:%d vnet:%d bitWidth:%d\n",
    m_net_ptr->MessageSizeType_to_int(net_msg_ptr->getMessageSize()),
    vnet, oPort->bitWidth());
    
    //if(vnet==4){
        //cout<<" flit num for VC4: "<<num_flits<<endl;
        //num_flits=4;
    //}
    
    // loop to convert all multicast messages into unicast messages
    for (int ctr = 0; ctr < dest_nodes.size(); ctr++) {

        // this will return a free output virtual channel
        int vc = calculateVC(vnet);

        if (vc == -1) {
            return false ;
        }
        MsgPtr new_msg_ptr = msg_ptr->clone();
        NodeID destID = dest_nodes[ctr];
        //cout<<"Node ID: "<< destID<<endl;
        
        Message *new_net_msg_ptr = new_msg_ptr.get();
        if (dest_nodes.size() > 1) {
            NetDest personal_dest;
            for (int m = 0; m < (int) MachineType_NUM; m++) {
                if ((destID >= MachineType_base_number((MachineType) m)) &&
                    destID < MachineType_base_number((MachineType) (m+1))) {
                    // calculating the NetDest associated with this destID
                    personal_dest.clear();
                    personal_dest.add((MachineID) {(MachineType) m, (destID -
                        MachineType_base_number((MachineType) m))});
                    new_net_msg_ptr->getDestination() = personal_dest;
                    break;
                }
            }
            net_msg_dest.removeNetDest(personal_dest);
            // removing the destination from the original message to reflect
            // that a message with this particular destination has been
            // flitisized and an output vc is acquired
            net_msg_ptr->getDestination().removeNetDest(personal_dest);
        }

        // Embed Route into the flits
        // NetDest format is used by the routing table
        // Custom routing algorithms just need destID

        RouteInfo route;
        route.vnet = vnet;
        route.net_dest = new_net_msg_ptr->getDestination();
        route.src_ni = m_id;
        route.src_router = oPort->routerID();
        route.dest_ni = destID;
        route.dest_router = m_net_ptr->get_router_id(destID, vnet);

        // initialize hops_traversed to -1
        // so that the first router increments it to 0
        route.hops_traversed = -1;

        m_net_ptr->increment_injected_packets(vnet);
        for (int i = 0; i < num_flits; i++) {
            m_net_ptr->increment_injected_flits(vnet);
            flit *fl = new flit(i, vc, vnet, route, num_flits, new_msg_ptr,
                m_net_ptr->MessageSizeType_to_int(
                net_msg_ptr->getMessageSize()),
                oPort->bitWidth(), curTick());

            fl->set_src_delay(curTick() - (msg_ptr->getTime()));
            m_ni_out_vcs[vc]->insert(fl);
        }

        m_ni_out_vcs_enqueue_time[vc] = curTick();
        m_out_vc_state[vc]->setState(ACTIVE_, curTick());
    }
    return true ;
}

// Looking for a free output vc
int
NetworkInterface::calculateVC(int vnet)
{
    for (int i = 0; i < m_vc_per_vnet; i++) {
        int delta = m_vc_allocator[vnet];
        m_vc_allocator[vnet]++;
        if (m_vc_allocator[vnet] == m_vc_per_vnet)
            m_vc_allocator[vnet] = 0;

        if (m_out_vc_state[(vnet*m_vc_per_vnet) + delta]->isInState(
                    IDLE_, curTick())) {
            vc_busy_counter[vnet] = 0;
            return ((vnet*m_vc_per_vnet) + delta);
        }
    }

    vc_busy_counter[vnet] += 1;
    /*panic_if(vc_busy_counter[vnet] > m_deadlock_threshold,
        "%s: Possible network deadlock in vnet: %d at time: %llu \n",
        name(), vnet, curTick());*/

    return -1;
}


// test free VC
bool
NetworkInterface::testFreeVC(int vnet)
{
    int tmp=0;
    for (int i = 0; i < m_vc_per_vnet; i++) {
        int delta = i;
        if (m_out_vc_state[(vnet*m_vc_per_vnet) + delta]->isInState(
                    IDLE_, curTick())) {
            tmp++;
            //return true;
        }
    }
    if((tmp-reserved_VCs[vnet])>0){
        return true;
    }
    return false;
}

bool
NetworkInterface::acquireFreeVC(int vnet)
{
    if(reserved_VCs[vnet]<m_vc_per_vnet){
        reserved_VCs[vnet]++;
        return true;
    }
    return false; 
}

/** This function looks at the NI buffers
 *  if some buffer has flits which are ready to traverse the link in the next
 *  cycle, and the downstream output vc associated with this flit has buffers
 *  left, the link is scheduled for the next cycle
 */
 void
NetworkInterface::scheduleOutputLink()
{
    for(int vnet=0;vnet<m_virtual_networks;vnet++){
        scheduleOutputLinkForVnet(vnet);
    }
}
 
void
NetworkInterface::scheduleOutputLinkForVnet(int vn)
{
    int vc = m_vc_round_robin_per_vnet[vn];
    m_vc_round_robin_per_vnet[vn]++;
    if (m_vc_round_robin_per_vnet[vn] == m_vc_per_vnet)
        m_vc_round_robin_per_vnet[vn] = 0;

    for (int i = 0; i < m_vc_per_vnet; i++) {
        vc++;
        if (vc == m_vc_per_vnet)
            vc = 0;
        int vc_for_vnet=vn*m_vc_per_vnet+vc;
        // model buffer backpressure
        if (m_ni_out_vcs[vc_for_vnet]->isReady(curTick()) &&
            m_out_vc_state[vc_for_vnet]->has_credit()) {

            bool is_candidate_vc = true;
            int t_vnet = vn;
            int vc_base = t_vnet * m_vc_per_vnet;

            //mycode
            //int maxVC=-1;
            //int max=-1;
            if (m_net_ptr->isVNetOrdered(t_vnet)) {
                for (int vc_offset = 0; vc_offset < m_vc_per_vnet;
                     vc_offset++) {
                    int t_vc = vc_base + vc_offset;
                    if (m_ni_out_vcs[t_vc]->isReady(curTick())) {
                        if (m_ni_out_vcs_enqueue_time[t_vc] <
                            m_ni_out_vcs_enqueue_time[vc_for_vnet]) {
                            is_candidate_vc = false;
                            break;
                        }
                        //mycode
                        /*if(m_ni_out_vcs_enqueue_time[t_vc]>max){
                            max=m_ni_out_vcs_enqueue_time[t_vc];
                            maxVC=t_vc;
                        }*/
                    }
                }
            }
            if (!is_candidate_vc)
                continue;

            m_out_vc_state[vc_for_vnet]->decrement_credit();
            // Just removing the flit
            flit *t_flit = m_ni_out_vcs[vc_for_vnet]->getTopFlit();
            
            //mycode
            /*if(t_flit->get_msg_ptr().get()->getStreamEnd()==true){
                
            }*/
            
            t_flit->set_time(clockEdge(Cycles(1)));
            scheduleFlit(t_flit);

            if (t_flit->get_type() == TAIL_ ||
               t_flit->get_type() == HEAD_TAIL_) {
                m_ni_out_vcs_enqueue_time[vc_for_vnet] = Tick(INFINITE_);
            }
            return;
        }
    }
}


/*void
NetworkInterface::scheduleOutputLink()
{
    int vc = m_vc_round_robin;
    m_vc_round_robin++;
    if (m_vc_round_robin == m_num_vcs)
        m_vc_round_robin = 0;

    for (int i = 0; i < m_num_vcs; i++) {
        vc++;
        if (vc == m_num_vcs)
            vc = 0;

        // model buffer backpressure
        if (m_ni_out_vcs[vc]->isReady(curTick()) &&
            m_out_vc_state[vc]->has_credit()) {

            bool is_candidate_vc = true;
            int t_vnet = get_vnet(vc);
            int vc_base = t_vnet * m_vc_per_vnet;

            if (m_net_ptr->isVNetOrdered(t_vnet)) {
                for (int vc_offset = 0; vc_offset < m_vc_per_vnet;
                     vc_offset++) {
                    int t_vc = vc_base + vc_offset;
                    if (m_ni_out_vcs[t_vc]->isReady(curTick())) {
                        if (m_ni_out_vcs_enqueue_time[t_vc] <
                            m_ni_out_vcs_enqueue_time[vc]) {
                            is_candidate_vc = false;
                            break;
                        }
                    }
                }
            }
            if (!is_candidate_vc)
                continue;

            m_out_vc_state[vc]->decrement_credit();
            // Just removing the flit
            flit *t_flit = m_ni_out_vcs[vc]->getTopFlit();
            t_flit->set_time(clockEdge(Cycles(1)));
            scheduleFlit(t_flit);

            if (t_flit->get_type() == TAIL_ ||
               t_flit->get_type() == HEAD_TAIL_) {
                m_ni_out_vcs_enqueue_time[vc] = Tick(INFINITE_);
            }
            return;
        }
    }
}*/

NetworkInterface::InputPort *
NetworkInterface::getInportForVnet(int vnet)
{
    for (auto &iPort : inPorts) {
        if (iPort->isVnetSupported(vnet)) {
            return iPort;
        }
    }

    return NULL;
}

NetworkInterface::OutputPort *
NetworkInterface::getOutportForVnet(int vnet)
{
    for (auto &oPort : outPorts) {
        if (oPort->isVnetSupported(vnet)) {
            return oPort;
        }
    }

    return NULL;
}
void
NetworkInterface::scheduleFlit(flit *t_flit)
{
    OutputPort *oPort = getOutportForVnet(t_flit->get_vnet());

    if (oPort) {
        DPRINTF(RubyNetwork, "Scheduling at %s time:%ld flit:%s Message:%s\n",
        oPort->outNetLink()->name(), clockEdge(Cycles(1)),
        *t_flit, *(t_flit->get_msg_ptr()));
        oPort->outFlitQueue()->insert(t_flit);
        oPort->outNetLink()->scheduleEventAbsolute(clockEdge(Cycles(1)));
        return;
    }

    fatal("No output port found for vnet:%d\n", t_flit->get_vnet());
    return;
}

int
NetworkInterface::get_vnet(int vc)
{
    for (int i = 0; i < m_virtual_networks; i++) {
        if (vc >= (i*m_vc_per_vnet) && vc < ((i+1)*m_vc_per_vnet)) {
            return i;
        }
    }
    fatal("Could not determine vc");
}


// Wakeup the NI in the next cycle if there are waiting
// messages in the protocol buffer, or waiting flits in the
// output VC buffer.
// Also check if we have to reschedule because of a clock period
// difference.
void
NetworkInterface::checkReschedule()
{
    if(events_list.find(curTick()+1)!=events_list.end()){
        return;
    }
    for (const auto& it : inNode_ptr) {
        if (it == nullptr) {
            continue;
        }

        while (it->isReady(clockEdge())) { // Is there a message waiting
            scheduleEvent(Cycles(1));
            return;
        }
    }

    for (int vc = 0; vc < m_num_vcs; vc++) {
        if (m_ni_out_vcs[vc]->isReady(clockEdge(Cycles(1)))) {
            scheduleEvent(Cycles(1));
            return;
        }
    }

    // Check if any input links have flits to be popped.
    // This can happen if the links are operating at
    // a higher frequency.
    for (auto &iPort : inPorts) {
        NetworkLink *inNetLink = iPort->inNetLink();
        if (inNetLink->isReady(curTick())) {
            scheduleEvent(Cycles(1));
            return;
        }
    }

    for (auto &oPort : outPorts) {
        CreditLink *inCreditLink = oPort->inCreditLink();
        if (inCreditLink->isReady(curTick())) {
            scheduleEvent(Cycles(1));
            return;
        }
    }
}

int NetworkInterface::sim_comm_size(sim_comm comm, int* size){
    return -1;
}
int NetworkInterface::sim_comm_get_rank(sim_comm comm, int *size){
    return -1;
}
int NetworkInterface::sim_comm_set_rank(sim_comm comm, int rank){
    return -1;
}
int NetworkInterface::sim_finish(){
    if(m_id!=0){
        return 0;
    }
    exitSimLoop("End of simulation");
    return 1;
}
double NetworkInterface::sim_time_resolution(){
    return -1;
}
int NetworkInterface::sim_init(){
    return -1;
}
timespec_t NetworkInterface::sim_get_time(){
    timespec_t tt;
    tt.time_res=time_type_e::NS;
    tt.time_val=curTick()*CLK_PERIOD;
    return tt;
}
void NetworkInterface::sim_schedule(timespec_t delta, void (*fun_ptr)(void *fun_arg), void *fun_arg){
    unsigned long long clks=delta.time_val/CLK_PERIOD;
    unsigned long long abs_clk=clks+curTick();
    bool wake_up=false;
    std::pair<void (*)(void *),void*> new_event=std::make_pair(fun_ptr,fun_arg);
    if(events_list.find(abs_clk)==events_list.end()){
        std::list<std::pair<void (*)(void *),void*>> empty;
        events_list[abs_clk]=empty;
        wake_up=true;
    }
    events_list[abs_clk].push_back(new_event);
    if(wake_up){
        scheduleEvent(Cycles(clks));
    }
    return;
}
int NetworkInterface::sim_send(void *buffer, int count, int type, int dst, int tag, sim_request *request){
    if(true) {
        //int flits=ceil(((double)m_net_ptr->get_package_packet_size()*8)/flit_width);
        //std::cout<<"send called from node: "<<m_id<<" , to node: "<<dst<<" , at vnet: "<<request->vnet<<" ,packet size: "<<m_net_ptr->get_package_packet_size()<<" , flits per packet: "<<flits<<std::endl;
    }
    assert(count>0);
    //count=nextPowerOf2(count);
    int packets=ceil(((double)count)/m_net_ptr->get_package_packet_size());
    Send_Req sr(count,packets,type,dst,tag,m_net_ptr->get_package_packet_size());
    send_reqs[request->vnet].push_back(sr);
    /*for(int i=0;i<packets;i++){
        int flits=ceil(((double)m_net_ptr->get_package_packet_size())/flit_width);
        MsgPtr new_msg_ptr;
        Message *new_net_msg_ptr;
        new_msg_ptr = template_msg->clone();
        new_net_msg_ptr = new_msg_ptr.get();
        NodeID destID=dst;
        NetDest personal_dest;
        for (int m = 0; m < (int) MachineType_NUM; m++) {
            if ((destID >= MachineType_base_number((MachineType) m)) &&
                destID < MachineType_base_number((MachineType) (m+1))) {
                // calculating the NetDest associated with this destID
                personal_dest.clear();
                personal_dest.add((MachineID) {(MachineType) m, (destID -
                                                                 MachineType_base_number((MachineType) m))});
                new_net_msg_ptr->getDestination() = personal_dest;
                break;
            }
        }
        new_net_msg_ptr->m_size=m_net_ptr->get_package_packet_size();
        new_net_msg_ptr->setFlits(flits);
        new_net_msg_ptr->message_end=true;
        new_net_msg_ptr->splitted=false;
        new_net_msg_ptr->capi_type=type;
        new_net_msg_ptr->capi_src=m_id;
        new_net_msg_ptr->capi_tag=tag;
        new_net_msg_ptr->vnet_to_fetch=request->vnet;
        send_reqs[request->vnet].push_back(new_msg_ptr);
    }*/
    //template_msg=template_msg->clone();
    //scheduleEvent(Cycles(1));
    if(events_list.find(curTick()+1)==events_list.end()){
        timespec_t delta;
        delta.time_val=1*CLK_PERIOD;
        sim_schedule(delta,&NetworkInterface::marker,NULL);
    }
    return 1;
}
MsgPtr NetworkInterface::create_packet(int packet_size,int type,int dst,int tag,bool is_end,int vnet){
    int flits=ceil(((double)packet_size*8)/flit_width);
    MsgPtr new_msg_ptr;
    Message *new_net_msg_ptr;
    new_msg_ptr = template_msg->clone();
    new_net_msg_ptr = new_msg_ptr.get();
    NodeID destID=dst;
    NetDest personal_dest;
    for (int m = 0; m < (int) MachineType_NUM; m++) {
        if ((destID >= MachineType_base_number((MachineType) m)) &&
            destID < MachineType_base_number((MachineType) (m+1))) {
            // calculating the NetDest associated with this destID
            personal_dest.clear();
            personal_dest.add((MachineID) {(MachineType) m, (destID -
                                                             MachineType_base_number((MachineType) m))});
            new_net_msg_ptr->getDestination() = personal_dest;
            break;
        }
    }
    new_net_msg_ptr->m_size=packet_size;
    new_net_msg_ptr->setFlits(flits);
    new_net_msg_ptr->message_end=is_end;
    new_net_msg_ptr->splitted=false;
    new_net_msg_ptr->capi_type=type;
    new_net_msg_ptr->capi_src=m_id;
    new_net_msg_ptr->capi_tag=tag;
    new_net_msg_ptr->vnet_to_fetch=vnet;
    return new_msg_ptr;
}
int NetworkInterface::sim_recv(void *buffer, int count, int type, int src, int tag, sim_request *request, void (*msg_handler)(void *fun_arg), void* fun_arg){
    if(true){
        //std::cout<<"recv called at node: "<<m_id<<" to wait for a packet from node: "<<src<<" , at vnet: "<<request->vnet<<std::endl;
    }
    //count=nextPowerOf2(count);
    int packets=ceil(((double)count)/m_net_ptr->get_package_packet_size());
    Recv_Req r(count,packets,type,src,tag,msg_handler,fun_arg);
    recv_reqs[request->vnet].push_back(r);
    return 1;
}
void NetworkInterface::call_events(){
    if(events_list.find(curTick())!=events_list.end()){
        std::list<std::pair<void (*)(void *),void*>> &events=events_list[curTick()];
        for(auto pair:events){
            (*(pair.first))(pair.second);
        }
        if(events.size()>0){
            events.clear();
        }
        events_list.erase(curTick());
    }
}
int NetworkInterface::nextPowerOf2(int n) {
    int count = 0;
    // First n in the below condition
    // is for the case where n is 0
    if (n && !(n & (n - 1)))
        return n;
    while( n != 0)
    {
        n >>= 1;
        count += 1;
    }
    assert(count>0);
    return 1 << (count-1);
}
void
NetworkInterface::print(std::ostream& out) const
{
    out << "[Network Interface]";
}

uint32_t
NetworkInterface::functionalWrite(Packet *pkt)
{
    uint32_t num_functional_writes = 0;
    for (unsigned int i  = 0; i < m_num_vcs; ++i) {
        num_functional_writes += m_ni_out_vcs[i]->functionalWrite(pkt);
    }

    for (auto &oPort: outPorts) {
        num_functional_writes += oPort->outFlitQueue()->functionalWrite(pkt);
    }
    return num_functional_writes;
}

NetworkInterface *
GarnetNetworkInterfaceParams::create()
{
    return new NetworkInterface(this);
}
