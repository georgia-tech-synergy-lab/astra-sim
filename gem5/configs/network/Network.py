# Copyright (c) 2016 Georgia Institute of Technology
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Tushar Krishna

import math
import m5
from m5.objects import *
from m5.defines import buildEnv
from m5.util import addToPath, fatal


def parse_network_input_options(options,var, val):
  if var=="num-npus:":
      options.num_cpus = int(val)
      options.num_dirs = int(val)
      print("num cpus is: "+str(val))
  elif var=="num-packages:":
      options.num_packages = int(val)
  elif var=="package-rows:":
      options.package_rows= int(val)
  elif var=="topology:":
      options.topology= str(val)
  elif var=="links-per-tile:":
    options.links_per_tile=int(val)
  elif var=="local-rings:":
    options.local_rings=int(val)
  elif var=="vertical-rings:":
    options.vertical_rings=int(val)
  elif var=="horizontal-rings:":
    options.horizontal_rings=int(val)
  if var=="flit-width:":
    options.ni_flit_size = int(val)
    options.flit_width = int(val)
  elif var=="local-packet-size:":
    options.local_packet_size=int(val)
  elif var=="package-packet-size:":
    options.package_packet_size=int(val)
  elif var=="tile-link-width:":
    options.tile_link_width=int(val)
  elif var=="package-link-width:":
    options.package_link_width=int(val)
  elif var=="vcs-per-vnet:":
    options.vcs_per_vnet = int(val)
  elif var=="routing-algorithm:":
    options.routing_algorithm=str(val)
  elif var=="router-latency:":
    options.router_latency=int(val)
  elif var=="local-link-latency:":
    options.local_link_latency=int(val)
  elif var=="package-link-latency:":
    options.package_link_latency=int(val)
  elif var=="buffers-per-vc:":
    options.buffers_per_ctrl_vc = int(val)
    options.buffers_per_data_vc = int(val)
  elif var=="local-link-efficiency:":
    options.local_link_efficiency = float(val)
  elif var=="package-link-efficiency:":
    options.package_link_efficiency = float(val)
def define_options(parser):
    # By default, ruby uses the simple timing cpu
    parser.set_defaults(cpu_type="TimingSimpleCPU")

    parser.add_option("--topology", type="string", default="Crossbar",
                      help="check configs/topologies for complete set")
    parser.add_option("--mesh-rows", type="int", default=0,
                      help="the number of rows in the mesh topology")
    parser.add_option("--network", type="choice", default="simple",
                      choices=['simple', 'garnet2.0'],
                      help="'simple'|'garnet2.0'")
    parser.add_option("--router-latency", action="store", type="int",
                      default=1,
                      help="""number of pipeline stages in the garnet router.
                            Has to be >= 1.
                            Can be over-ridden on a per router basis
                            in the topology file.""")
    parser.add_option("--local-link-latency", action="store", type="int", default=1,
                      help="""latency of each link the simple/garnet networks.
                            Has to be >= 1.
                            Can be over-ridden on a per link basis
                            in the topology file.""")
    parser.add_option("--package-link-latency", action="store", type="int", default=1,
                      help="""latency of each package link the simple/garnet networks.
                            Has to be >= 1.
                            Can be over-ridden on a per link basis
                            in the topology file.""")
    parser.add_option("--chiplet-link-latency", action="store", type="int", default=1,
                      help="""latency of chiplet link in garnet networks.
                            Has to be >= 1.
                            Can be over-ridden on a per link basis
                            in the topology file.""")
    parser.add_option("--chiplet-link-width", action="store", type="int",
                      default=128,
                      help="default width in bits for links inside garnet.")
    parser.add_option("--interposer-link-latency", action="store", type="int", default=1,
                      help="""latency of interposer link in garnet networks.
                            Has to be >= 1.
                            Can be over-ridden on a per link basis
                            in the topology file.""")
    parser.add_option("--interposer-link-width", action="store", type="int",
                      default=32,
                      help="default width in bits for links inside garnet.")
    parser.add_option("--clip-logic-ifc-delay", action="store", type="int", default=1,
                      help="delay due to chiplet logical protocol interface")
    parser.add_option("--clip-phys-ifc-delay", action="store", type="int", default=1,
                      help="delay due to chiplet physical interface")
    parser.add_option("--local-parallel-reduce", action="store", type="int", default=1,
                      help="reduction")
    parser.add_option("--vertical-parallel-reduce", action="store", type="int", default=1,
                      help="reduction")
    parser.add_option("--horizontal-parallel-reduce", action="store", type="int", default=1,
                      help="reduction")
    parser.add_option("--perpendicular-parallel-reduce", action="store", type="int", default=1,
                      help="reduction")
    parser.add_option("--fourth-parallel-reduce", action="store", type="int", default=1,
                      help="reduction")

    parser.add_option("--flit-width", action="store", type="int", default=128,
                      help="flit size in bits")
    parser.add_option("--tile-link-width", action="store", type="int", default=128,
                      help="tile link size in bits")
    parser.add_option("--num-packages", action="store", type="int", default=1,
                      help="tile link size in bits")
    parser.add_option("--local-rings", action="store", type="int", default=1,
                      help="local rings between tiles of same packages")
    parser.add_option("--vertical-rings", action="store", type="int", default=1,
                      help="vertical rings")
    parser.add_option("--horizontal-rings", action="store", type="int", default=1,
                      help="horizontal rings")
    parser.add_option("--perpendicular-rings", action="store", type="int", default=0,
                      help="perpendicular rings")
    parser.add_option("--fourth-rings", action="store", type="int", default=0,
                      help="perpendicular rings")

    parser.add_option("--package-rows", action="store", type="int", default=2,
                      help="horizontal rings")
    parser.add_option("--package-cols", action="store", type="int", default=0,
                      help="horizontal rings")
    parser.add_option("--package-height", action="store", type="int", default=0,
                      help="horizontal rings")

    parser.add_option("--package-link-width", action="store", type="int", default=128,
                      help="package link size in bits")
    parser.add_option("--local-burst-length", type="int", default=10,
                      help="Burst length.")
    parser.add_option("--vertical-burst-length", type="int", default=10,
                      help="Burst length.")
    parser.add_option("--horizontal-burst-length", type="int", default=10,
                      help="Burst length.")
    parser.add_option("--perpendicular-burst-length", type="int", default=10,
                      help="Burst length.")
    parser.add_option("--fourth-burst-length", type="int", default=10,
                      help="Burst length.")
    parser.add_option("--local-packet-size", type="int", default=64,
                      help="Burst length.")
    parser.add_option("--package-packet-size", type="int", default=64,
                      help="Burst length.")
    parser.add_option("--local-message-size", type="int", default=10,
                      help="Burst length.")
    parser.add_option("--horizontal-message-size", type="int", default=10,
                      help="Burst length.")
    parser.add_option("--vertical-message-size", type="int", default=10,
                      help="Burst length.")
    parser.add_option("--perpendicular-message-size", type="int", default=10,
                      help="Burst length.")
    parser.add_option("--fourth-message-size", type="int", default=10,
                      help="Burst length.")
    parser.add_option("--messaging-agent-buffer-size", type="int", default=2048,
                      help="Burst length.")

    parser.add_option("--burst-interval", type="int", default=100,
                      help="Burst Interval.")
    parser.add_option("--chunk-numbers", type="int", default=1,
                      help="chunk#.")
    parser.add_option("--processing-delay", type="int", default=0,
                      help="processing delay per 64B")
    parser.add_option("--communication-delay", type="int", default=10,
                      help="communication delay per 64B")
    parser.add_option("--vcs-per-vnet", action="store", type="int", default=4,
                      help="""number of virtual channels per virtual network
                            inside garnet network.""")
    parser.add_option("--buffers-per-ctrl-vc", action="store", type="int", default=16,
                      help="number of buffers in each ctrl VC.")
    parser.add_option("--buffers-per-data-vc", action="store", type="int", default=32,
                      help="number of buffers in each data VC.")
    parser.add_option("--boost-mode", action="store", type="int", default=False,
                      help="number of buffers in each data VC.")
    parser.add_option("--time-boost", action="store", type="int", default=False,
                      help="number of buffers in each data VC.")
    parser.add_option("--routing-algorithm", type="choice",
                      default="table",
                      choices=['Mesh_XY', 'Ring_XY', 'table','AllToAll'],
                      help="""routing algorithm in network.
                            'table' | 'xy' | 'turn_model_oblivious' |
                            'turn_model_adaptive' | 'random_oblivious' |
                            'random_adaptive' | 'custom'.
                            Implementation: see garnet2.0/RoutingUnit.cc""")
    parser.add_option("--algorithm", type="choice",
                      default="4phase",
                      choices=['3phase','4phase', '5phase'],
                      help="""algorithm for all-reduce""")
    parser.add_option("--method", type="choice",
                      default="baseline",
                      choices=['baseline', 'proposed'],
                      help="""method for all-reduce""")
    parser.add_option("--network-fault-model", action="store_true",
                      default=False,
                      help="""enable network fault model:
                            see src/mem/ruby/network/fault_model/""")
    parser.add_option("--garnet-deadlock-threshold", action="store",
                      type="int", default=50000,
                      help="network-level deadlock threshold.")
    parser.add_option("--local-link-efficiency", type="float", default=1,
                  help="Injection rate in packets per cycle per node. \
                        Takes decimal value between 0 to 1 (eg. 0.225). \
                        Number of digits after 0 depends upon --precision.")
    parser.add_option("--package-link-efficiency", type="float", default=1,
                  help="Injection rate in packets per cycle per node. \
                        Takes decimal value between 0 to 1 (eg. 0.225). \
                        Number of digits after 0 depends upon --precision.")
    parser.add_option("--compute-scale", type="float", default=1,
                  help="Injection rate in packets per cycle per node. \
                        Takes decimal value between 0 to 1 (eg. 0.225). \
                        Number of digits after 0 depends upon --precision.")
    parser.add_option("--comm-scale", type="float", default=1,
                  help="Injection rate in packets per cycle per node. \
                        Takes decimal value between 0 to 1 (eg. 0.225). \
                        Number of digits after 0 depends upon --precision.")
    parser.add_option("--local-reduction-delay", type="int", default=0,
                      help="communication delay per 64B")
    parser.add_option("--model-shared-bus", type="int", default=1,
                      help="shared bus")
    parser.add_option("--L", type="int", default=75,
                      help="communication delay per 64B")
    parser.add_option("--o", type="int", default=20,
                            help="communication delay per 64B")
    parser.add_option("--g", type="int", default=50,
                      help="communication delay per 64B")
    parser.add_option("--G", type="float", default=0.0033,
                      help="communication delay per 64B")
    parser.add_option("--workload", type="string", default="test_workload",
                      help="check configs/topologies for complete set")
    parser.add_option("--sys", type="string", default="sample_sys",
                      help="check configs/topologies for complete set")
    parser.add_option("--net", type="string", default="sample_net",
                      help="check configs/topologies for complete set")
    parser.add_option("--scheduling-policy", type="choice",
                      default="LIFO",
                      choices=['LIFO', 'FIFO'],
                      help="""algorithm for all-reduce""")
    parser.add_option("--preferred-dataset-splits", type="int", default=16,
                            help="communication delay per 64B")
    parser.add_option("--num-passes", type="int", default=2,
                            help="toal number of passes")
    parser.add_option("--packet-routing", type="choice",
                      default="software",
                      choices=['software', 'hardware'],
                      help="""algorithm for all-reduce""")
    parser.add_option("--injection-policy", type="choice",
                      default="aggressive",
                      choices=['aggressive', 'normal'],
                      help="""algorithm for all-reduce""")
    parser.add_option("--links-per-tile", type="int", default=1,
                            help="communication delay per 64B")
    parser.add_option("--num-alus", type="int", default=8,
                            help="communication delay per 64B")
    parser.add_option("--num-backward-links", type="int", default=8,
                            help="communication delay per 64B")
    parser.add_option("--total-stat-rows", type="int", default=8,
                            help="communication delay per 64B")
    parser.add_option("--stat-row", type="int", default=8,
                            help="communication delay per 64B")
    parser.add_option("--path", type="string", default="",
                      help="check configs/topologies for complete set")
    parser.add_option("--run-name", type="string", default="test",
                      help="check configs/topologies for complete set")
    parser.add_option("--fifo-size", type="int", default=2097152,
                      help="communication delay per 64B")
    parser.add_option("--ram-size", type="int", default=262144,
                      help="communication delay per 64B")
    
def create_network(options, ruby):
    try:
      netInput = open("network_inputs/"+options.net+".txt", "r") 
      print("Success in opening net file!")
      index=0
      inps=["",""]  
      with netInput as f:
        for line in f:
          for word in line.split():
            inps[index%2]=word
            index+=1
            if index%2==0:
              parse_network_input_options(options,inps[0],inps[1])        
    except IOError:
      print("Could not open net file!")
    options.network="garnet2.0"
    options.synthetic="training"

    # Set the network classes based on the command line options
    NetworkClass = GarnetNetwork
    IntLinkClass = GarnetIntLink
    ExtLinkClass = GarnetExtLink
    RouterClass = GarnetRouter
    InterfaceClass = GarnetNetworkInterface
    # Instantiate the network object
    # so that the controllers can connect to it.
    network = NetworkClass(ruby_system = ruby, topology = options.topology,
            routers = [], ext_links = [], int_links = [], netifs = [])

    return (network, IntLinkClass, ExtLinkClass, RouterClass, InterfaceClass)


def init_network(options, network, InterfaceClass):
   
    try:
      netInput = open("network_inputs/"+options.net+".txt", "r") 
      print("Success in opening net file!")
      index=0
      inps=["",""]  
      with netInput as f:
        for line in f:
          for word in line.split():
            inps[index%2]=word
            index+=1
            if index%2==0:
              parse_network_input_options(options,inps[0],inps[1])
              
    except IOError:
      print("Could not open net file!")

    network.num_cpus = options.num_cpus 
    network.num_packages = options.num_packages
    network.package_rows = options.package_rows 
    network.topology=options.topology

    network.links_per_tile=options.links_per_tile
    network.local_rings=options.local_rings
    network.vertical_rings=options.vertical_rings
    network.horizontal_rings=options.horizontal_rings
    network.perpendicular_rings=options.perpendicular_rings
    network.fourth_rings=options.fourth_rings

    network.ni_flit_size = options.ni_flit_size
    network.flit_width = options.flit_width

    network.local_packet_size = options.local_packet_size
    network.package_packet_size= options.package_packet_size

    network.vcs_per_vnet = options.vcs_per_vnet
    network.buffers_per_ctrl_vc = options.buffers_per_ctrl_vc
    network.buffers_per_data_vc = options.buffers_per_data_vc
    network.local_link_efficiency = options.local_link_efficiency
    network.package_link_efficiency = options.package_link_efficiency

    network.num_rows = options.mesh_rows
      #options.chiplet_link_width
    #network.routing_algorithm = options.routing_algorithm
    network.garnet_deadlock_threshold = options.garnet_deadlock_threshold
    #mycode
    network.traffic_type = options.synthetic
    #network.parallel_reduce = options.local_parallel_reduce
    #network.vertical_parallel_reduce = options.vertical_parallel_reduce
    network.messaging_agent_buffer_size=options.messaging_agent_buffer_size

    network.local_parallel_reduce=options.local_parallel_reduce
    network.vertical_parallel_reduce=options.vertical_parallel_reduce
    network.horizontal_parallel_reduce=options.horizontal_parallel_reduce
    network.perpendicular_parallel_reduce=options.perpendicular_parallel_reduce
    network.fourth_parallel_reduce=options.fourth_parallel_reduce

    
    network.package_cols=options.package_cols
    network.package_height=options.package_height

    network.local_burst_length=options.local_burst_length
    network.horizontal_burst_length=options.horizontal_burst_length
    network.vertical_burst_length=options.vertical_burst_length
    network.perpendicular_burst_length=options.perpendicular_burst_length
    network.fourth_burst_length=options.fourth_burst_length


    network.local_message_size=options.local_message_size
    network.horizontal_message_size=options.horizontal_message_size
    network.vertical_message_size=options.vertical_message_size
    network.perpendicular_message_size=options.perpendicular_message_size
    network.fourth_message_size=options.fourth_message_size

    network.burst_interval=options.burst_interval
    network.chunk_numbers=options.chunk_numbers
    network.processing_delay=options.processing_delay
    network.algorithm=options.algorithm
    network.method=options.method
    network.communication_delay=options.communication_delay
    network.workload=options.workload
    network.sys_input=options.sys
    network.scheduling_policy=options.scheduling_policy
    network.compute_scale=options.compute_scale
    network.comm_scale=options.comm_scale
    network.local_reduction_delay=options.local_reduction_delay
    network.model_shared_bus=options.model_shared_bus
    network.preferred_dataset_splits=options.preferred_dataset_splits
    network.num_passes=options.num_passes
    network.packet_routing=options.packet_routing
    network.injection_policy=options.injection_policy
  
    network.num_alus=options.num_alus
    network.num_backward_links=options.num_backward_links
    
    network.ram_size=options.ram_size
    network.fifo_size=options.fifo_size

    network.L=options.L
    network.o=options.o
    network.g=options.g
    network.G=options.G

    network.boost_mode=options.boost_mode
    network.time_boost=options.time_boost

    network.total_stat_rows=options.total_stat_rows
    network.stat_row=options.stat_row
    network.path=options.path
    network.run_name=options.run_name


    if options.routing_algorithm == "table":
       network.routing_algorithm = 0
    elif options.routing_algorithm == "Mesh_XY":
        network.routing_algorithm = 1
    elif options.routing_algorithm == "Ring_XY":
        network.routing_algorithm = 3
    elif options.routing_algorithm == "AllToAll":
        network.routing_algorithm = 4
    else:
        network.routing_algorithm = 0

    if True: #options.routing_algorithm == "table":
        #network.routing_algorithm = 0
        # Create bridge and connect them to the corresponding links
        for intLink in network.int_links:
            intLink.tx_net_bridge = CLIP(
                                     link = intLink.network_link,
                                     vtype = 1,
                                     width = intLink.src_node.width,
                                     logic_latency=options.clip_logic_ifc_delay,
                                     phys_latency=options.clip_phys_ifc_delay)
            intLink.tx_cred_bridge = CLIP(
                                    link = intLink.credit_link,
                                    vtype = 0,
                                    width = intLink.src_node.width)
            intLink.rx_net_bridge = CLIP(
                                    link = intLink.network_link,
                                    vtype = 0,
                                    width = intLink.dst_node.width,
                                    logic_latency=options.clip_logic_ifc_delay,
                                    phys_latency=options.clip_phys_ifc_delay)
            intLink.rx_cred_bridge = CLIP(
                                    link = intLink.credit_link,
                                    vtype = 1,
                                    width = intLink.dst_node.width)

        for extLink in network.ext_links:
            nic_net_bridges = []
            nic_net_bridges.append(CLIP(link =
                                 extLink.network_links[0], vtype = 1,
                                 width = extLink.width))
            nic_net_bridges.append(CLIP(link =
                                 extLink.network_links[1], vtype = 0,
                                 width = extLink.width))
            extLink.nic_net_bridge = nic_net_bridges

            nic_credit_bridges = []
            nic_credit_bridges.append(CLIP(link =
                                    extLink.credit_links[0], vtype = 0,
                                    width = extLink.width))
            nic_credit_bridges.append(CLIP(link =
                                    extLink.credit_links[1], vtype = 1,
                                    width = extLink.width))
            extLink.nic_cred_bridge = nic_credit_bridges

            rtr_net_bridges = []
            rtr_net_bridges.append(CLIP(link =
                                 extLink.network_links[0], vtype = 0,
                                 width = extLink.int_node.width))
            rtr_net_bridges.append(CLIP(link =
                                 extLink.network_links[1], vtype = 1,
                                 width = extLink.int_node.width))
            extLink.rtr_net_bridge = rtr_net_bridges

            rtr_cred_bridges = []
            rtr_cred_bridges.append(CLIP(link =
                                  extLink.credit_links[0], vtype = 1,
                                  width = extLink.int_node.width))
            rtr_cred_bridges.append(CLIP(link =
                                  extLink.credit_links[1], vtype = 0,
                                  width = extLink.int_node.width))
            extLink.rtr_cred_bridge = rtr_cred_bridges


    if InterfaceClass != None:
        netifs = [InterfaceClass(id=i) \
                  for (i,n) in enumerate(network.ext_links)]
        network.netifs = netifs

    if options.network_fault_model:
        assert(options.network == "garnet2.0")
        network.enable_fault_model = True
        network.fault_model = FaultModel()
