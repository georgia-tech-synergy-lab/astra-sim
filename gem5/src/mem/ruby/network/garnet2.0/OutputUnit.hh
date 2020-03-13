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


#ifndef __MEM_RUBY_NETWORK_GARNET2_0_OUTPUTUNIT_HH__
#define __MEM_RUBY_NETWORK_GARNET2_0_OUTPUTUNIT_HH__

#include <iostream>
#include <vector>

#include "mem/ruby/common/Consumer.hh"
#include "mem/ruby/network/garnet2.0/CommonTypes.hh"
#include "mem/ruby/network/garnet2.0/CreditLink.hh"
#include "mem/ruby/network/garnet2.0/NetworkLink.hh"
#include "mem/ruby/network/garnet2.0/OutVcState.hh"
#include "mem/ruby/network/garnet2.0/Router.hh"
#include "mem/ruby/network/garnet2.0/flitBuffer.hh"
#include "mem/ruby/network/garnet2.0/InputUnit.hh"
#include <boost/algorithm/string/predicate.hpp>

class OutputUnit : public Consumer
{
  public:
    OutputUnit(int id, PortDirection direction, Router *router);
    ~OutputUnit();
    void set_out_link(NetworkLink *link);
    void set_credit_link(CreditLink *credit_link);
    void wakeup();
    flitBuffer* getOutQueue();
    void print(std::ostream& out) const {};
    void decrement_credit(int out_vc);
    void increment_credit(int out_vc);
    bool has_credit(int out_vc);

    //int reduced_VCs;
    //int base_offset;
    bool has_free_vc(int vnet);
    bool has_free_vc(int vnet, int invc,PortDirection inport_dirn, PortDirection outport_dirn, RouteInfo route, bool splitted);

    int select_free_vc(int vnet);
    int select_free_vc(int vnet, int invc,PortDirection inport_dirn, PortDirection outport_dirn, RouteInfo route, bool splitted);

    inline PortDirection get_direction() { return m_direction; }

    int
    get_credit_count(int vc)
    {
        return m_outvc_state[vc]->get_credit_count();
    }

    inline int
    get_outlink_id()
    {
        return m_out_link->get_id();
    }

    inline void
    set_vc_state(VC_state_type state, int vc, Tick curTime)
    {
      m_outvc_state[vc]->setState(state, curTime);
    }

    inline bool
    is_vc_idle(int vc, Tick curTime)
    {
        return (m_outvc_state[vc]->isInState(IDLE_, curTime));
    }

    inline void
    insert_flit(flit *t_flit)
    {
        m_out_buffer->insert(t_flit);
        m_out_link->scheduleEventAbsolute(m_router->clockEdge(Cycles(1)));
    }
    void find_peer(int offset){
        int node_id=m_router->get_id();
        int peer_id=-1;

        int cpus=m_router->get_net_ptr()->get_num_cpus();
        int packs=m_router->get_net_ptr()->get_num_packages();
        int rows=m_router->get_net_ptr()->get_package_rows();
        //int cols=m_router->get_net_ptr()->get_package_cols();
        //int cpus=m_router->get_net_ptr()->get_package_height();

        int local_dim=cpus/packs;
        int horizontal_dim=packs/rows;
        int vertical_dim=rows;

        int my_local=node_id%local_dim;
        int my_horizontal=(node_id%(local_dim*horizontal_dim))/local_dim;
        int my_vertical=node_id/(local_dim*horizontal_dim);

        //int my_local_orig=node_id%local_dim;
        //int my_horizontal_orig=(node_id%(local_dim*horizontal_dim))/local_dim;
        //int my_vertical_orig=node_id/(local_dim*horizontal_dim);

        if(boost::starts_with(m_direction,"LocalEast")){
            if(my_local==0 && local_dim>2){
                critical=true;
            }
            my_local--;
            if(my_local<0){
                my_local=local_dim-1;
            }

        }
        else if(boost::starts_with(m_direction,"East")){
            if(my_horizontal==0 && horizontal_dim>2){
                critical=true;
            }
            my_horizontal--;
            if(my_horizontal<0){
                my_horizontal=horizontal_dim-1;
            }
        }
        else if(boost::starts_with(m_direction,"West")){
            if(my_horizontal==0 && horizontal_dim>2){
                critical=true;
            }
            my_horizontal++;
            if(my_horizontal==horizontal_dim){
                my_horizontal=0;
            }
        }
        else if(boost::starts_with(m_direction,"North")){
            if(my_vertical==0 && vertical_dim>2){
                critical=true;
            }
            my_vertical--;
            if(my_vertical<0){
                my_vertical=vertical_dim-1;
            }
        }
        else if(boost::starts_with(m_direction,"South")){
            if(my_vertical==0 && vertical_dim>2){
                critical=true;
            }
            my_vertical++;
            if(my_vertical==vertical_dim){
                my_vertical=0;
            }
        }
        peer_id=my_local+(my_horizontal*local_dim)+(my_vertical*(local_dim*horizontal_dim));
        if(node_id!=peer_id){
            for(auto &pou:Router::all_routers[peer_id]->get_outputUnit_ref()){
                if(pou->get_direction()==m_direction){
                    peer=pou;
                }
            }
            //std::cout<<"I am node: "<<node_id<<" ,with local dim: "<<my_local_orig<<" ,horiz dim: "<<my_horizontal_orig<<" ,vertic dim: "
            //<<my_vertical_orig<<" ,and my peer for port: "<<m_direction<<" is node: "<<peer_id<<" with port: "<<peer->get_direction()
            //<<" ,and the critically is: "<<std::boolalpha << critical <<std::endl;
        }
    }
    OutputUnit *peer;
    bool critical;
    uint32_t functionalWrite(Packet *pkt);
    Router *get_router(){return m_router;};
  private:
    int m_id;
    PortDirection m_direction;
    int m_num_vcs;
    int m_vc_per_vnet;
    Router *m_router;
    NetworkLink *m_out_link;
    CreditLink *m_credit_link;

    flitBuffer *m_out_buffer; // This is for the network link to consume
    std::vector<OutVcState *> m_outvc_state; // vc state of downstream router

};

#endif // __MEM_RUBY_NETWORK_GARNET2_0_OUTPUTUNIT_HH__
