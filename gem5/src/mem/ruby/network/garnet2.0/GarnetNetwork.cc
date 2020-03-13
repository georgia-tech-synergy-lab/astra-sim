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


#include "mem/ruby/network/garnet2.0/GarnetNetwork.hh"

#include <cassert>
#include <iostream>

#include "base/cast.hh"
#include "base/stl_helpers.hh"
#include "debug/RubyNetwork.hh"
#include "mem/ruby/common/NetDest.hh"
#include "mem/ruby/network/MessageBuffer.hh"
#include "mem/ruby/network/garnet2.0/CommonTypes.hh"
#include "mem/ruby/network/garnet2.0/CreditLink.hh"
#include "mem/ruby/network/garnet2.0/GarnetLink.hh"
#include "mem/ruby/network/garnet2.0/NetworkInterface.hh"
#include "mem/ruby/network/garnet2.0/NetworkLink.hh"
#include "mem/ruby/network/garnet2.0/Router.hh"
#include "mem/ruby/system/RubySystem.hh"

using namespace std;
using m5::stl_helpers::deletePointers;

/*
 * GarnetNetwork sets up the routers and links and collects stats.
 * Default parameters (GarnetNetwork.py) can be overwritten from command line
 * (see configs/network/Network.py)
 */

GarnetNetwork::GarnetNetwork(const Params *p)
    : Network(p)
{
    m_num_rows = p->num_rows;
    m_ni_flit_size = p->ni_flit_size;
    m_vcs_per_vnet = p->vcs_per_vnet;
    m_buffers_per_data_vc = p->buffers_per_data_vc;
    m_buffers_per_ctrl_vc = p->buffers_per_ctrl_vc;
    m_routing_algorithm = p->routing_algorithm;

    m_enable_fault_model = p->enable_fault_model;
    if (m_enable_fault_model)
        fault_model = p->fault_model;

    m_vnet_type.resize(m_virtual_networks);
    std::cout<<"garnet vns: "<<m_virtual_networks<<std::endl;
    
    //m_vnet_type_names[2]="ctrl";
    for (int i = 0 ; i < m_virtual_networks ; i++) {
        if (m_vnet_type_names[i] == "response"){
            m_vnet_type[i] = DATA_VNET_; // carries data (and ctrl) packets
            //std::cout<<"for vnet: "<<i<<" data type is selected"<<std::endl;
        }
        else{
            m_vnet_type[i] = CTRL_VNET_; // carries only ctrl packets
            //std::cout<<"for vnet: "<<i<<" ctrl type is selected"<<std::endl;
        }
      
    }
    
    // record the routers
    for (vector<BasicRouter*>::const_iterator i =  p->routers.begin();
         i != p->routers.end(); ++i) {
        Router* router = safe_cast<Router*>(*i);
        m_routers.push_back(router);

        // initialize the router's network pointers
        router->init_net_ptr(this);
    }
    // record the network interfaces
    for (vector<ClockedObject*>::const_iterator i = p->netifs.begin();
         i != p->netifs.end(); ++i) {
        NetworkInterface *ni = safe_cast<NetworkInterface *>(*i);
        m_nis.push_back(ni);
        ni->init_net_ptr(this);
    }
	
	//mycode
	//traffic_type=p->traffic_type;
	//parallel_reduce=p->parallel_reduce;
    //vertical_parallel_reduce=p->vertical_parallel_reduce;
    //num_packages=p->num_packages;
    
    traffic_type=p->traffic_type;
    local_parallel_reduce=p->local_parallel_reduce;
    vertical_parallel_reduce=p->vertical_parallel_reduce;
    horizontal_parallel_reduce=p->horizontal_parallel_reduce;
    perpendicular_parallel_reduce=p->perpendicular_parallel_reduce;
    fourth_parallel_reduce=p->fourth_parallel_reduce;
    
    local_rings=p->local_rings;
    vertical_rings=p->vertical_rings;
    horizontal_rings=p->horizontal_rings;
    perpendicular_rings=p->perpendicular_rings;
    fourth_rings=p->fourth_rings;
    
    num_cpus=p->num_cpus;
    num_packages=p->num_packages;
    package_rows=p->package_rows;
    package_cols=p->package_cols;
    package_height=p->package_height;
    
    local_burst_length=p->local_burst_length;
    horizontal_burst_length=p->horizontal_burst_length;
    vertical_burst_length=p->vertical_burst_length;
    perpendicular_burst_length=p->perpendicular_burst_length;
    fourth_burst_length=p->fourth_burst_length;
    
    local_packet_size=p->local_packet_size;
    package_packet_size=p->package_packet_size;
    
    local_message_size=p->local_message_size;
    horizontal_message_size=p->horizontal_message_size;
    vertical_message_size=p->vertical_message_size;
    perpendicular_message_size=p->perpendicular_message_size;
    fourth_message_size=p->fourth_message_size;
    messaging_agent_buffer_size=p->messaging_agent_buffer_size;
    
    burst_interval=p->burst_interval;
    flit_width=p->flit_width;
    num_packages=p->num_packages;
    chunk_numbers=p->chunk_numbers;
    processing_delay=p->processing_delay;
    algorithm=p->algorithm;
    method=p->method;
    communication_delay=p->communication_delay;
    topology=p->topology;
    if(p->topology=="NV_Switch"){
        isNVSwitch=true;
    }
    else{
        isNVSwitch=false;
    }
    workload=p->workload;
    sys_input=p->sys_input;
    scheduling_policy=p->scheduling_policy;
    
    local_link_efficiency=p->local_link_efficiency;
    package_link_efficiency=p->package_link_efficiency;
    
    boost_mode=p->boost_mode==0?false:true;
    time_boost=p->time_boost==0?false:true;
    model_shared_bus=p->model_shared_bus==0?false:true;
    preferred_dataset_splits=p->preferred_dataset_splits;

    packet_routing=p->packet_routing;
    injection_policy=p->injection_policy;

    compute_scale=p->compute_scale;
    comm_scale=p->comm_scale;
    local_reduction_delay=p->local_reduction_delay;

    L=p->L;
    o=p->o;
    g=p->g;
    G=p->G;

    num_passes=p->num_passes;
    links_per_tile=p->links_per_tile;

    num_alus=p->num_alus;
    num_backward_links=p->num_backward_links;

    total_stat_rows=p->total_stat_rows;
    stat_row=p->stat_row;
    path=p->path;
    run_name=p->run_name;

    fifo_size=p->fifo_size;
    ram_size=p->ram_size;
    //std::cout<<"garnet network finished "<<std::endl;
	//cout << "traffic : "<< p->traffic_type << endl;
}

void
GarnetNetwork::init()
{
    Network::init();

    for (int i=0; i < m_nodes; i++) {
        m_nis[i]->addNode(m_toNetQueues[i], m_fromNetQueues[i]);
    }

    // The topology pointer should have already been initialized in the
    // parent network constructor
    assert(m_topology_ptr != NULL);
    m_topology_ptr->createLinks(this);

    // Initialize topology specific parameters
//    if (getNumRows() > 0) {
//        // Only for Mesh topology
//        // m_num_rows and m_num_cols are only used for
//        // implementing XY or custom routing in RoutingUnit.cc
//        m_num_rows = getNumRows();
//        m_num_cols = m_routers.size() / m_num_rows;
//        assert(m_num_rows * m_num_cols == m_routers.size());
//    } else {
        m_num_rows = -1;
        m_num_cols = -1;
//    }

    // FaultModel: declare each router to the fault model
    if (isFaultModelEnabled()) {
        for (vector<Router*>::const_iterator i= m_routers.begin();
             i != m_routers.end(); ++i) {
            Router* router = safe_cast<Router*>(*i);
            int router_id M5_VAR_USED =
                fault_model->declare_router(router->get_num_inports(),
                                            router->get_num_outports(),
                                            router->get_vc_per_vnet(),
                                            getBuffersPerDataVC(),
                                            getBuffersPerCtrlVC());
            assert(router_id == router->get_id());
            router->printAggregateFaultProbability(cout);
            router->printFaultVector(cout);
        }
    }
    
    //mycode
    int vnets=0;
    for(int i=0;i<local_rings;i++){
        local_vnets.push_back(vnets++);
    }
    if(topology=="AllToAll" || topology=="NV_Switch" ){
        for(int i=0;i<get_num_packages()*horizontal_rings;i++){
            horizontal_vnets1.push_back(vnets++);
        }
        return;
    }
    for(int i=0;i<vertical_rings;i++){
        vertical_vnets1.push_back(vnets++);
    }
    for(int i=0;i<vertical_rings;i++){
        vertical_vnets2.push_back(vnets++);
    }
    for(int i=0;i<horizontal_rings;i++){
        horizontal_vnets1.push_back(vnets++);
    }
    for(int i=0;i<horizontal_rings;i++){
        horizontal_vnets2.push_back(vnets++);
    }
    if(topology=="Torus4D" || topology=="Torus5D" ){
        for(int i=0;i<perpendicular_rings;i++){
            perpendicular_vnets1.push_back(vnets++);
        }
        for(int i=0;i<perpendicular_rings;i++){
            perpendicular_vnets2.push_back(vnets++);
        }
        if(topology=="Torus4D"){
            for(int i=0;i<fourth_rings;i++){
                fourth_vnets1.push_back(vnets++);
            }
            for(int i=0;i<fourth_rings;i++){
                fourth_vnets2.push_back(vnets++);
            }
        }
    }
}

GarnetNetwork::~GarnetNetwork()
{
    deletePointers(m_routers);
    deletePointers(m_nis);
    deletePointers(m_networklinks);
    deletePointers(m_creditlinks);
}

/*
 * This function creates a link from the Network Interface (NI)
 * into the Network.
 * It creates a Network Link from the NI to a Router and a Credit Link from
 * the Router to the NI
*/

void
GarnetNetwork::makeExtInLink(NodeID src, SwitchID dest, BasicLink* link,
                            std::vector<NetDest>& routing_table_entry)
{
    assert(src < m_nodes);

    GarnetExtLink* garnet_link = safe_cast<GarnetExtLink*>(link);

    // GarnetExtLink is bi-directional
    NetworkLink* net_link = garnet_link->m_network_links[LinkDirection_In];
    net_link->setType(EXT_IN_);
    CreditLink* credit_link = garnet_link->m_credit_links[LinkDirection_In];

    m_networklinks.push_back(net_link);
    m_creditlinks.push_back(credit_link);

    PortDirection dst_inport_dirn = "Local";
    ClockedObject *extNode = garnet_link->params()->ext_node;
    m_nis[src]->setClockDomain(extNode->getClockDomain());

    if (garnet_link->nicClipEn) {
        DPRINTF(RubyNetwork, "Enable CLIP at NIC for %s\n",
            garnet_link->name());
        m_nis[src]->
        addOutPort(garnet_link->nicNetBridge[LinkDirection_In],
                   garnet_link->nicCredBridge[LinkDirection_In],
                   dest);
    } else {
        m_nis[src]->addOutPort(net_link, credit_link, dest);
    }

    if (garnet_link->rtrClipEn) {
        DPRINTF(RubyNetwork, "Enable CLIP at Rtr for %s\n",
            garnet_link->name());
        m_routers[dest]->
            addInPort(dst_inport_dirn,
                      garnet_link->rtrNetBridge[LinkDirection_In],
                      garnet_link->rtrCredBridge[LinkDirection_In]);
    } else {
        m_routers[dest]->addInPort(dst_inport_dirn, net_link, credit_link);
    }

}

/*
 * This function creates a link from the Network to a NI.
 * It creates a Network Link from a Router to the NI and
 * a Credit Link from NI to the Router
*/

void
GarnetNetwork::makeExtOutLink(SwitchID src, NodeID dest, BasicLink* link,
                             std::vector<NetDest>& routing_table_entry)
{
    assert(dest < m_nodes);
    assert(src < m_routers.size());
    assert(m_routers[src] != NULL);

    GarnetExtLink* garnet_link = safe_cast<GarnetExtLink*>(link);

    // GarnetExtLink is bi-directional
    NetworkLink* net_link = garnet_link->m_network_links[LinkDirection_Out];
    net_link->setType(EXT_OUT_);
    CreditLink* credit_link = garnet_link->m_credit_links[LinkDirection_Out];

    m_networklinks.push_back(net_link);
    m_creditlinks.push_back(credit_link);

    PortDirection src_outport_dirn = "Local";

    if (garnet_link->nicClipEn) {
        DPRINTF(RubyNetwork, "Enable CLIP at NIC for %s\n",
            garnet_link->name());
        m_nis[dest]->
            addInPort(garnet_link->nicNetBridge[LinkDirection_Out],
                      garnet_link->nicCredBridge[LinkDirection_Out]);
    } else {
        m_nis[dest]->addInPort(net_link, credit_link);
    }

    if (garnet_link->rtrClipEn) {
        DPRINTF(RubyNetwork, "Enable CLIP at Rtr for %s\n",
            garnet_link->name());
        m_routers[src]->
            addOutPort(src_outport_dirn,
                       garnet_link->rtrNetBridge[LinkDirection_Out],
                       routing_table_entry, link->m_weight,
                       garnet_link->rtrCredBridge[LinkDirection_Out]);
    } else {
        m_routers[src]->
            addOutPort(src_outport_dirn, net_link,
                       routing_table_entry,
                       link->m_weight, credit_link);
    }
}

/*
 * This function creates an internal network link between two routers.
 * It adds both the network link and an opposite credit link.
*/

void
GarnetNetwork::makeInternalLink(SwitchID src, SwitchID dest, BasicLink* link,
                                std::vector<NetDest>& routing_table_entry,
                                PortDirection src_outport_dirn,
                                PortDirection dst_inport_dirn)
{
    GarnetIntLink* garnet_link = safe_cast<GarnetIntLink*>(link);

    // GarnetIntLink is unidirectional
    NetworkLink* net_link = garnet_link->m_network_link;
    net_link->setType(INT_);
    CreditLink* credit_link = garnet_link->m_credit_link;

    m_networklinks.push_back(net_link);
    m_creditlinks.push_back(credit_link);

    if (garnet_link->rxClipEn) {
        DPRINTF(RubyNetwork, "Enable CLIP at Rx for %s\n",
            garnet_link->name());
        m_routers[dest]->addInPort(dst_inport_dirn,
            garnet_link->rxNetBridge, garnet_link->rxCredBridge);
    } else {
        m_routers[dest]->addInPort(dst_inport_dirn, net_link, credit_link);
    }

    if (garnet_link->txClipEn) {
        DPRINTF(RubyNetwork, "Enable CLIP at Tx for %s\n",
            garnet_link->name());
        m_routers[src]->
            addOutPort(src_outport_dirn, garnet_link->txNetBridge,
                       routing_table_entry,
                       link->m_weight, garnet_link->txCredBridge);
    } else {
        m_routers[src]->addOutPort(src_outport_dirn, net_link,
                        routing_table_entry,
                        link->m_weight, credit_link);
    }
}

// Total routers in the network
int
GarnetNetwork::getNumRouters()
{
    return m_routers.size();
}

// Get ID of router connected to a NI.
int
GarnetNetwork::get_router_id(int ni, int vnet)
{
    return m_nis[ni]->get_router_id(vnet);
}

void
GarnetNetwork::regStats()
{
    Network::regStats();

    // Packets
    m_packets_received
        .init(m_virtual_networks)
        .name(name() + ".packets_received")
        .flags(Stats::pdf | Stats::total | Stats::nozero | Stats::oneline)
        ;

    m_packets_injected
        .init(m_virtual_networks)
        .name(name() + ".packets_injected")
        .flags(Stats::pdf | Stats::total | Stats::nozero | Stats::oneline)
        ;

    m_packet_network_latency
        .init(m_virtual_networks)
        .name(name() + ".packet_network_latency")
        .flags(Stats::oneline)
        ;

    m_packet_queueing_latency
        .init(m_virtual_networks)
        .name(name() + ".packet_queueing_latency")
        .flags(Stats::oneline)
        ;

    for (int i = 0; i < m_virtual_networks; i++) {
        m_packets_received.subname(i, csprintf("vnet-%i", i));
        m_packets_injected.subname(i, csprintf("vnet-%i", i));
        m_packet_network_latency.subname(i, csprintf("vnet-%i", i));
        m_packet_queueing_latency.subname(i, csprintf("vnet-%i", i));
    }

    m_avg_packet_vnet_latency
        .name(name() + ".average_packet_vnet_latency")
        .flags(Stats::oneline);
    m_avg_packet_vnet_latency =
        m_packet_network_latency / m_packets_received;

    m_avg_packet_vqueue_latency
        .name(name() + ".average_packet_vqueue_latency")
        .flags(Stats::oneline);
    m_avg_packet_vqueue_latency =
        m_packet_queueing_latency / m_packets_received;

    m_avg_packet_network_latency
        .name(name() + ".average_packet_network_latency");
    m_avg_packet_network_latency =
        sum(m_packet_network_latency) / sum(m_packets_received);

    m_avg_packet_queueing_latency
        .name(name() + ".average_packet_queueing_latency");
    m_avg_packet_queueing_latency
        = sum(m_packet_queueing_latency) / sum(m_packets_received);

    m_avg_packet_latency
        .name(name() + ".average_packet_latency");
    m_avg_packet_latency
        = m_avg_packet_network_latency + m_avg_packet_queueing_latency;

    // Flits
    m_flits_received
        .init(m_virtual_networks)
        .name(name() + ".flits_received")
        .flags(Stats::pdf | Stats::total | Stats::nozero | Stats::oneline)
        ;

    m_flits_injected
        .init(m_virtual_networks)
        .name(name() + ".flits_injected")
        .flags(Stats::pdf | Stats::total | Stats::nozero | Stats::oneline)
        ;

    m_flit_network_latency
        .init(m_virtual_networks)
        .name(name() + ".flit_network_latency")
        .flags(Stats::oneline)
        ;

    m_flit_queueing_latency
        .init(m_virtual_networks)
        .name(name() + ".flit_queueing_latency")
        .flags(Stats::oneline)
        ;

    for (int i = 0; i < m_virtual_networks; i++) {
        m_flits_received.subname(i, csprintf("vnet-%i", i));
        m_flits_injected.subname(i, csprintf("vnet-%i", i));
        m_flit_network_latency.subname(i, csprintf("vnet-%i", i));
        m_flit_queueing_latency.subname(i, csprintf("vnet-%i", i));
    }

    m_avg_flit_vnet_latency
        .name(name() + ".average_flit_vnet_latency")
        .flags(Stats::oneline);
    m_avg_flit_vnet_latency = m_flit_network_latency / m_flits_received;

    m_avg_flit_vqueue_latency
        .name(name() + ".average_flit_vqueue_latency")
        .flags(Stats::oneline);
    m_avg_flit_vqueue_latency =
        m_flit_queueing_latency / m_flits_received;

    m_avg_flit_network_latency
        .name(name() + ".average_flit_network_latency");
    m_avg_flit_network_latency =
        sum(m_flit_network_latency) / sum(m_flits_received);

    m_avg_flit_queueing_latency
        .name(name() + ".average_flit_queueing_latency");
    m_avg_flit_queueing_latency =
        sum(m_flit_queueing_latency) / sum(m_flits_received);

    m_avg_flit_latency
        .name(name() + ".average_flit_latency");
    m_avg_flit_latency =
        m_avg_flit_network_latency + m_avg_flit_queueing_latency;


    // Hops
    m_avg_hops.name(name() + ".average_hops");
    m_avg_hops = m_total_hops / sum(m_flits_received);

    // Links
    m_total_ext_in_link_utilization
        .name(name() + ".ext_in_link_utilization");
    m_total_ext_out_link_utilization
        .name(name() + ".ext_out_link_utilization");
    m_total_int_link_utilization
        .name(name() + ".int_link_utilization");
    m_average_link_utilization
        .name(name() + ".avg_link_utilization");

    m_average_vc_load
        .init(m_virtual_networks * m_vcs_per_vnet)
        .name(name() + ".avg_vc_load")
        .flags(Stats::pdf | Stats::total | Stats::nozero | Stats::oneline)
        ;
}

void
GarnetNetwork::collateStats()
{
    RubySystem *rs = params()->ruby_system;
    double time_delta = double(curCycle() - rs->getStartCycle());

    for (int i = 0; i < m_networklinks.size(); i++) {
        link_type type = m_networklinks[i]->getType();
        int activity = m_networklinks[i]->getLinkUtilization();

        if (type == EXT_IN_)
            m_total_ext_in_link_utilization += activity;
        else if (type == EXT_OUT_)
            m_total_ext_out_link_utilization += activity;
        else if (type == INT_)
            m_total_int_link_utilization += activity;

        m_average_link_utilization +=
            (double(activity) / time_delta);

        vector<unsigned int> vc_load = m_networklinks[i]->getVcLoad();
        for (int j = 0; j < vc_load.size(); j++) {
            m_average_vc_load[j] += ((double)vc_load[j] / time_delta);
        }
    }

    // Ask the routers to collate their statistics
    for (int i = 0; i < m_routers.size(); i++) {
        m_routers[i]->collateStats();
    }
}

void
GarnetNetwork::print(ostream& out) const
{
    out << "[GarnetNetwork]";
}

GarnetNetwork *
GarnetNetworkParams::create()
{
    return new GarnetNetwork(this);
}

/*
 * The Garnet Network has an array of routers. These routers have buffers
 * that need to be accessed for functional reads and writes. Also the links
 * between different routers have buffers that need to be accessed.
*/
bool
GarnetNetwork::functionalRead(Packet * pkt)
{
    for(unsigned int i = 0; i < m_routers.size(); i++) {
        if (m_routers[i]->functionalRead(pkt)) {
            return true;
        }
    }

    return false;

}

uint32_t
GarnetNetwork::functionalWrite(Packet *pkt)
{
    uint32_t num_functional_writes = 0;

    for (unsigned int i = 0; i < m_routers.size(); i++) {
        num_functional_writes += m_routers[i]->functionalWrite(pkt);
    }

    for (unsigned int i = 0; i < m_nis.size(); ++i) {
        num_functional_writes += m_nis[i]->functionalWrite(pkt);
    }

    for (unsigned int i = 0; i < m_networklinks.size(); ++i) {
        num_functional_writes += m_networklinks[i]->functionalWrite(pkt);
    }

    return num_functional_writes;
}
