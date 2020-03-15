#Copyright (c) 2020 Georgia Institute of Technology
#Permission is hereby granted, free of charge, to any person obtaining a copy
#of this software and associated documentation files (the "Software"), to deal
#in the Software without restriction, including without limitation the rights
#to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#copies of the Software, and to permit persons to whom the Software is
#furnished to do so, subject to the following conditions:
#The above copyright notice and this permission notice shall be included in all
#copies or substantial portions of the Software.
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#SOFTWARE.

#Author : Saeed Rashidi (saeed.rashidi@gatech.edu)

from m5.params import *
from m5.objects import *
import math 

from BaseTopology import SimpleTopology

# Creates a generic Mesh assuming an equal number of cache
# and directory controllers.
# XY routing is enforced (using link weights)
# to guarantee deadlock freedom.

class NV_Switch(SimpleTopology):
    description='Torus'

    def __init__(self, controllers):
        self.nodes = controllers

    # Makes a generic mesh
    # assuming an equal number of cache and directory cntrls

    def makeTopology(self, options, network, IntLink, ExtLink, Router):
        nodes = self.nodes

        num_routers = options.num_cpus
        num_rows = options.num_packages
        links_per_tile= options.links_per_tile
        
        num_global_rows = options.package_rows
        num_global_cols = int(options.num_packages/num_global_rows)
        num_local_cols = int(options.num_cpus/options.num_packages)
        
        local_vnets = range(0,options.local_rings)
        print "local vnets:" + str(local_vnets)
        global_vnets = range(options.local_rings,options.local_rings+num_rows*options.horizontal_rings)
        all_vnets = range(0,options.local_rings+links_per_tile*options.horizontal_rings)
        print "all vnets:" + str(all_vnets)
        
        flit_width = options.flit_width
        tile_link_width = options.tile_link_width
        package_link_width = options.package_link_width
        
        packageEqFlit = (package_link_width != flit_width)
        tileEqFlit = (tile_link_width != flit_width)
        
        # default values for link latency and router latency.
        # Can be over-ridden on a per link/router basis
        local_link_latency = options.local_link_latency # used by simple and garnet
        package_link_latency = options.package_link_latency # used by simple and garnet
        router_latency = options.router_latency # only used by garnet

        #print "number of nodes: " + str(len(nodes))
        
        # There must be an evenly divisible number of cntrls to routers
        # Also, obviously the number or rows must be <= the number of routers
        cntrls_per_router, remainder = divmod(len(nodes), num_routers)
        assert(num_rows > 0 and num_rows <= num_routers)
        num_columns = int(num_routers / num_rows)
        assert(num_columns * num_rows == num_routers)

        # Create the routers in the mesh
        routers = [Router(router_id=i, latency = router_latency) \
            for i in range(num_routers+links_per_tile)] #range(num_routers+num_columns*(links_per_tile))
        network.routers = routers

        # link counter to set unique link ids
        link_count = 0

        # Add all but the remainder nodes to the list of nodes to be uniformly
        # distributed across the network.
        network_nodes = []
        remainder_nodes = []
        for node_index in xrange(len(nodes)):  
            if node_index < (len(nodes) - remainder):
                network_nodes.append(nodes[node_index])
            else:
                remainder_nodes.append(nodes[node_index])
        
        
        # Connect each node to the appropriate router
        ext_links = []
        for (i, n) in enumerate(network_nodes): 
            cntrl_level, router_id = divmod(i, num_routers)
            routers[router_id].width = flit_width
            assert(cntrl_level < cntrls_per_router)
            for j in all_vnets:
                ext_links.append(ExtLink(link_id=link_count, ext_node=n,
                                int_node=routers[router_id],
                                supported_vnets = [j],
                                latency = local_link_latency,
                                width = flit_width
                                ))
                link_count += 1
            
        # Connect the remainding nodes to router 0.  These should only be
        # DMA nodes.
        for (i, node) in enumerate(remainder_nodes):
            assert(node.type == 'DMA_Controller')
            assert(i < remainder)
            print "hey"
            ext_links.append(ExtLink(link_id=link_count, ext_node=node,
                                    int_node=routers[0],
                                    latency = link_latency,
                                    width=flit_width))
            link_count += 1

        network.ext_links = ext_links

        # Create the mesh links.
        int_links = []

        #East output to West input links (weight = 1)
        for row in xrange(num_rows):
            for col in xrange(num_columns):
                if (col + 1 <= num_columns and num_columns > 1):
                    east_out = col + (row * num_columns)
                    west_in = ((col + 1) % num_columns) + (row * num_columns)
                    for j in local_vnets:
                        print "Router " + get_id(routers[east_out]) + " created a link to Router " +  get_id(routers[west_in])+" for vnet: "+str(j)+" with lid: "+str(link_count)
                        int_links.append(IntLink(link_id=link_count,
                                             src_node=routers[east_out],
                                             dst_node=routers[west_in],
                                             src_outport="LocalEast"+str(j),
                                             dst_inport="LocalWest"+str(j),
                                             latency = local_link_latency,
                                             width = tile_link_width,
                                             tx_clip = tileEqFlit,
                                             rx_clip = tileEqFlit,
                                             weight=1,
                                             supported_vnets = [j]))
                        link_count += 1
        
        for local in xrange(num_columns):
            for row_out in xrange(num_rows):
                nodeport = (row_out * num_columns) + local
                for j in xrange(links_per_tile):
                    switchport = num_routers + j # + local*(links_per_tile)
                    print "Router " + get_id(routers[nodeport]) + " created a link to Router " +  get_id(routers[switchport])+" with lid: "+str(link_count)
                    int_links.append(IntLink(link_id=link_count,
                                    src_node=routers[nodeport],
                                    dst_node=routers[switchport],
                                    src_outport="Out"+str(j),             #+str(options.local_rings+j*num_rows+row_in),
                                    dst_inport="In"+str(nodeport),               #+str(options.local_rings+j*num_rows+row_in),
                                    latency = package_link_latency,
                                    width = package_link_width,
                                    tx_clip = packageEqFlit,
                                    rx_clip = packageEqFlit,
                                    weight=1,
                                    supported_vnets =-1))
                    link_count += 1
                    print "Router " + get_id(routers[switchport]) + " created a link to Router " +  get_id(routers[nodeport])+" with lid: "+str(link_count)  
                    int_links.append(IntLink(link_id=link_count,
                                    src_node=routers[switchport],
                                    dst_node=routers[nodeport],
                                    src_outport="Out"+str(nodeport),                     #+str(options.local_rings+j*num_rows+row_out),
                                    dst_inport="In"+str(nodeport),                            #+str(options.local_rings+j*num_rows+row_out),
                                    latency = package_link_latency,
                                    width = package_link_width,
                                    tx_clip = packageEqFlit,
                                    rx_clip = packageEqFlit,
                                    weight=1,
                                    supported_vnets =-1))
                    link_count += 1
                    
        network.int_links = int_links 
def get_id(node) :
    return str(node).split('.')[3].split('routers')[1]
