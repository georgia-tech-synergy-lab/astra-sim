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


#ifndef __MEM_RUBY_NETWORK_GARNET2_0_GARNETNETWORK_HH__
#define __MEM_RUBY_NETWORK_GARNET2_0_GARNETNETWORK_HH__

#include <iostream>
#include <vector>

#include "mem/ruby/network/Network.hh"
#include "mem/ruby/network/fault_model/FaultModel.hh"
#include "mem/ruby/network/garnet2.0/CommonTypes.hh"
#include "params/GarnetNetwork.hh"

//mycode
//#include "cpu/testers/garnet_synthetic_traffic/GarnetSyntheticTraffic.hh"


class FaultModel;
class NetworkInterface;
class Router;
class NetDest;
class NetworkLink;
class CreditLink;

class GarnetNetwork : public Network
{
  public:
    typedef GarnetNetworkParams Params;
    GarnetNetwork(const Params *p);

    ~GarnetNetwork();
    void init();

    // Configuration (set externally)

    // for 2D topology
    int getNumRows() const { return m_num_rows; }
    int getNumCols() { return m_num_cols; }

    // for network
    uint32_t getNiFlitSize() const { return m_ni_flit_size; }
    uint32_t getVCsPerVnet() const { return m_vcs_per_vnet; }
    uint32_t getBuffersPerDataVC() { return m_buffers_per_data_vc; }
    uint32_t getBuffersPerCtrlVC() { return m_buffers_per_ctrl_vc; }
    int getRoutingAlgorithm() const { return m_routing_algorithm; }

    bool isFaultModelEnabled() const { return m_enable_fault_model; }
    FaultModel* fault_model;

	//myccode
	bool isItTraining(){ return traffic_type=="training"; }
	//int getParallelReduce(){ return parallel_reduce; }
    //int getVerticalParallelReduce(){ return vertical_parallel_reduce; }
    int get_num_cpus(){ return num_cpus; }
    int get_num_packages(){ return num_packages; }
    
    int get_local_parallel_reduce(){ return local_parallel_reduce; }
    int get_vertical_parallel_reduce(){ return vertical_parallel_reduce; }
    int get_horizontal_parallel_reduce(){ return horizontal_parallel_reduce; }
    int get_perpendicular_parallel_reduce(){ return perpendicular_parallel_reduce; }
    int get_fourth_parallel_reduce(){ return fourth_parallel_reduce; }
    
    int get_local_rings(){ return local_rings; }
    int get_vertical_rings(){ return vertical_rings; }
    int get_horizontal_rings(){ return horizontal_rings; }
    int get_perpendicular_rings(){ return perpendicular_rings; }
    int get_fourth_rings(){ return fourth_rings; }
    
    int get_package_rows(){ return package_rows; }
    int get_package_cols(){ return package_cols; }
    int get_package_height(){ return package_height; }
    
    int get_local_burst_length(){ return local_burst_length; }
    int get_horizontal_burst_length(){ return horizontal_burst_length; }
    int get_vertical_burst_length(){ return vertical_burst_length; }
    int get_perpendicular_burst_length(){ return perpendicular_burst_length; }
    int get_fourth_burst_length(){ return fourth_burst_length; }
    
    int get_local_packet_size(){ return local_packet_size; }
    int get_package_packet_size(){ return package_packet_size; }
    
    bool get_boost_mode(){ return boost_mode; }
    bool get_time_boost(){ return time_boost; }
    
    int get_local_message_size(){ return local_message_size; }
    int get_horizontal_message_size(){ return horizontal_message_size; }
    int get_vertical_message_size(){ return vertical_message_size; }
    int get_perpendicular_message_size(){ return perpendicular_message_size; }
    int get_fourth_message_size(){ return fourth_message_size; }
    int get_messaging_agent_buffer_size(){ return messaging_agent_buffer_size; }
    
    int get_burst_interval(){ return burst_interval; }
    int get_flit_width(){ return flit_width; }
    int get_chunk_numbers(){ return chunk_numbers;}
    int get_processing_delay(){ return processing_delay;}
    int get_communication_delay(){ return communication_delay;}
    std::string get_algorithm(){return algorithm;}
    std::string get_topology(){return topology;}
    std::string get_method(){return method;}
    std::string get_workload(){return workload;}
    std::string get_scheduling_policy(){return scheduling_policy;}

    float get_compute_scale(){return compute_scale;}
    float get_comm_scale(){return comm_scale;}
    int get_local_reduction_delay(){return local_reduction_delay;}


    std::vector<int> local_vnets;
    std::vector<int> vertical_vnets1;
    std::vector<int> vertical_vnets2;
    std::vector<int> horizontal_vnets1;
    std::vector<int> horizontal_vnets2;
    std::vector<int> perpendicular_vnets1;
    std::vector<int> perpendicular_vnets2;
    std::vector<int> fourth_vnets1;
    std::vector<int> fourth_vnets2;
    std::string packet_routing;
    std::string injection_policy;
    std::string sys_input;
    int L;
    int o;
    int g;
    float G;
    bool model_shared_bus;
    int preferred_dataset_splits;
    int num_passes;
    bool isNVSwitch;
    int links_per_tile;
    int num_cpus;
    int num_alus;
    int num_backward_links;
    std::string run_name;
    int fifo_size;
    int ram_size;

    int total_stat_rows;
    int stat_row;
    std::string path;
    
    float get_local_link_efficiency(){ return local_link_efficiency;}
    float get_package_link_efficiency(){ return package_link_efficiency;}
    
    
	//int getNumberOfNodes(){ return m_routers.size(); };
	
    // Internal configuration
    bool isVNetOrdered(int vnet) const { return true; }//return m_ordered[vnet]; }
    VNET_type
    get_vnet_type(int vc)
    {
        int vnet = vc/getVCsPerVnet();
        return m_vnet_type[vnet];
    }
    int getNumRouters();
    int get_router_id(int ni, int vnet);


    // Methods used by Topology to setup the network
    void makeExtOutLink(SwitchID src, NodeID dest, BasicLink* link,
                     std::vector<NetDest>& routing_table_entry);
    void makeExtInLink(NodeID src, SwitchID dest, BasicLink* link,
                    std::vector<NetDest>& routing_table_entry);
    void makeInternalLink(SwitchID src, SwitchID dest, BasicLink* link,
                          std::vector<NetDest>& routing_table_entry,
                          PortDirection src_outport_dirn,
                          PortDirection dest_inport_dirn);

    bool functionalRead(Packet * pkt);
    //! Function for performing a functional write. The return value
    //! indicates the number of messages that were written.
    uint32_t functionalWrite(Packet *pkt);

    // Stats
    void collateStats();
    void regStats();
    void print(std::ostream& out) const;

    // increment counters
    void increment_injected_packets(int vnet) { m_packets_injected[vnet]++; }
    void increment_received_packets(int vnet) { m_packets_received[vnet]++; }

    void
    increment_packet_network_latency(Tick latency, int vnet)
    {
        m_packet_network_latency[vnet] += latency;
    }

    void
    increment_packet_queueing_latency(Tick latency, int vnet)
    {
        m_packet_queueing_latency[vnet] += latency;
    }

    void increment_injected_flits(int vnet) { m_flits_injected[vnet]++; }
    void increment_received_flits(int vnet) { m_flits_received[vnet]++; }

    void
    increment_flit_network_latency(Tick latency, int vnet)
    {
        m_flit_network_latency[vnet] += latency;
    }

    void
    increment_flit_queueing_latency(Tick latency, int vnet)
    {
        m_flit_queueing_latency[vnet] += latency;
    }

    void
    increment_total_hops(int hops)
    {
        m_total_hops += hops;
    }

  protected:
    // Configuration
    int m_num_rows;
    int m_num_cols;
    uint32_t m_ni_flit_size;
    uint32_t m_vcs_per_vnet;
    uint32_t m_buffers_per_ctrl_vc;
    uint32_t m_buffers_per_data_vc;
    int m_routing_algorithm;
    bool m_enable_fault_model;

    // Statistical variables
    Stats::Vector m_packets_received;
    Stats::Vector m_packets_injected;
    Stats::Vector m_packet_network_latency;
    Stats::Vector m_packet_queueing_latency;

    Stats::Formula m_avg_packet_vnet_latency;
    Stats::Formula m_avg_packet_vqueue_latency;
    Stats::Formula m_avg_packet_network_latency;
    Stats::Formula m_avg_packet_queueing_latency;
    Stats::Formula m_avg_packet_latency;

    Stats::Vector m_flits_received;
    Stats::Vector m_flits_injected;
    Stats::Vector m_flit_network_latency;
    Stats::Vector m_flit_queueing_latency;

    Stats::Formula m_avg_flit_vnet_latency;
    Stats::Formula m_avg_flit_vqueue_latency;
    Stats::Formula m_avg_flit_network_latency;
    Stats::Formula m_avg_flit_queueing_latency;
    Stats::Formula m_avg_flit_latency;

    Stats::Scalar m_total_ext_in_link_utilization;
    Stats::Scalar m_total_ext_out_link_utilization;
    Stats::Scalar m_total_int_link_utilization;
    Stats::Scalar m_average_link_utilization;
    Stats::Vector m_average_vc_load;

    Stats::Scalar  m_total_hops;
    Stats::Formula m_avg_hops;

  private:
    GarnetNetwork(const GarnetNetwork& obj);
    GarnetNetwork& operator=(const GarnetNetwork& obj);

    std::vector<VNET_type > m_vnet_type;
    std::vector<Router *> m_routers;   // All Routers in Network
    std::vector<NetworkLink *> m_networklinks; // All flit links in the network
    std::vector<CreditLink *> m_creditlinks; // All credit links in the network
    std::vector<NetworkInterface *> m_nis;   // All NI's in Network
	
	//mycode
	std::string traffic_type;
	
    int local_parallel_reduce;
    int vertical_parallel_reduce;
    int horizontal_parallel_reduce;
    int perpendicular_parallel_reduce;
    int fourth_parallel_reduce;
    
    int local_rings;
    int vertical_rings;
    int horizontal_rings;
    int perpendicular_rings;
    int fourth_rings;

    int num_packages;
    int package_rows;
    int package_cols;
    int package_height;
    
    int local_burst_length;
    int horizontal_burst_length;
    int vertical_burst_length;
    int perpendicular_burst_length;
    int fourth_burst_length;
    
    int local_packet_size;
    int package_packet_size;
    
    int local_message_size;
    int horizontal_message_size;
    int vertical_message_size;
    int perpendicular_message_size;
    int fourth_message_size;
    
    int burst_interval;
    int flit_width;
    int chunk_numbers;
    int processing_delay;
    std::string algorithm;
    std::string topology;
    std::string method;
    std::string workload;
    std::string scheduling_policy;

    float compute_scale;
    float comm_scale;
    int local_reduction_delay;

    int communication_delay;
    
    bool boost_mode;
    bool time_boost;
    
    float local_link_efficiency;
    float package_link_efficiency;
    int messaging_agent_buffer_size;
    
    
};

inline std::ostream&
operator<<(std::ostream& out, const GarnetNetwork& obj)
{
    obj.print(out);
    out << std::flush;
    return out;
}

#endif //__MEM_RUBY_NETWORK_GARNET2_0_GARNETNETWORK_HH__
