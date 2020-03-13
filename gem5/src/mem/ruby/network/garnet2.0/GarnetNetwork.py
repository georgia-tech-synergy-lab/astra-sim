# Copyright (c) 2008 Princeton University
# Copyright (c) 2009 Advanced Micro Devices, Inc.
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
# Author: Tushar Krishna
#

from m5.params import *
from m5.proxy import *
from m5.objects.Network import RubyNetwork
from m5.objects.BasicRouter import BasicRouter
from m5.objects.ClockedObject import ClockedObject

class GarnetNetwork(RubyNetwork):
    type = 'GarnetNetwork'
    cxx_header = "mem/ruby/network/garnet2.0/GarnetNetwork.hh"
    num_rows = Param.Int(0, "number of rows if 2D (mesh/torus/..) topology");
    ni_flit_size = Param.UInt32(16, "network interface flit size in bytes")
    vcs_per_vnet = Param.UInt32(4, "virtual channels per virtual network");
    buffers_per_data_vc = Param.UInt32(4, "buffers per data virtual channel");
    buffers_per_ctrl_vc = Param.UInt32(1, "buffers per ctrl virtual channel");
    routing_algorithm = Param.Int(0,
        "0: Weight-based Table, 1: XY, 2: Custom");
    enable_fault_model = Param.Bool(False, "enable network fault model");
    fault_model = Param.FaultModel(NULL, "network fault model");
    garnet_deadlock_threshold = Param.UInt32(50000,
                              "network-level deadlock threshold");
    #mycode
    #traffic_type = Param.String("uniform_random", "Traffic type")
    #parallel_reduce = Param.Int(1, "parallel reduce")
    #vertical_parallel_reduce = Param.Int(1, "parallel reduce")
    #num_packages = Param.Int(1, "parallel reduce")

    traffic_type = Param.String("uniform_random", "Traffic type")
    num_packages = Param.Int(1, "parallel reduce")

    local_parallel_reduce = Param.Int(1, "")
    vertical_parallel_reduce = Param.Int(1, "")
    horizontal_parallel_reduce = Param.Int(1, "")
    perpendicular_parallel_reduce = Param.Int(1, "")
    fourth_parallel_reduce = Param.Int(1, "")

    local_rings = Param.Int(1, "")
    vertical_rings = Param.Int(1, "")
    horizontal_rings = Param.Int(1, "")
    perpendicular_rings = Param.Int(0, "")
    fourth_rings = Param.Int(0, "")

    num_cpus = Param.Int(1, "")
    num_packages = Param.Int(1, "")
    package_rows = Param.Int(1, "")
    package_cols = Param.Int(0, "")
    package_height = Param.Int(0, "")

    local_burst_length = Param.Int(1, "")
    horizontal_burst_length = Param.Int(1, "")
    vertical_burst_length = Param.Int(1, "")
    perpendicular_burst_length = Param.Int(1, "")
    fourth_burst_length = Param.Int(1, "")

    local_packet_size = Param.Int(1, "")
    package_packet_size = Param.Int(1, "")

    local_message_size = Param.Int(1, "")
    horizontal_message_size = Param.Int(1, "")
    vertical_message_size = Param.Int(1, "")
    perpendicular_message_size = Param.Int(1, "")
    fourth_message_size = Param.Int(1, "")
    messaging_agent_buffer_size=Param.Int(1024, "")

    burst_interval = Param.Int(1, "")
    flit_width = Param.Int(1, "")
    chunk_numbers=Param.Int(1, "")
    processing_delay=Param.Int(1, "")
    communication_delay=Param.Int(10, "")
    algorithm = Param.String("4phase", "alg")
    method = Param.String("baseline", "alg")
    topology = Param.String("Torus2D", "topo")
    workload = Param.String("test_workload", "workload")
    sys_input = Param.String("sample_sys", "sys_input")
    scheduling_policy = Param.String("LIFO", "scheduling")
    compute_scale=Param.Float(1, "")
    comm_scale=Param.Float(1, "")
    local_reduction_delay=Param.Int(1, "")
    model_shared_bus=Param.Int(1, "")
    preferred_dataset_splits=Param.Int(16, "")
    num_passes=Param.Int(2, "")
    packet_routing = Param.String("software", "Traffic type")
    injection_policy = Param.String("aggressive", "Traffic type")
    num_alus=Param.Int(8, "")
    num_backward_links=Param.Int(8, "")

    fifo_size=Param.Int(8, "")
    ram_size=Param.Int(8, "")

    L=Param.Int(1, "")
    o=Param.Int(1, "")
    g=Param.Int(1, "")
    G=Param.Float(1, "")
    links_per_tile=Param.Int(2, "")

    local_link_efficiency=Param.Float(1, "")
    package_link_efficiency=Param.Float(1, "")

    boost_mode=Param.Int(0, "")
    time_boost=Param.Int(0, "")

    total_stat_rows=Param.Int(0, "")
    stat_row=Param.Int(0, "")
    path=Param.String("", "alg")
    run_name=Param.String("", "alg")


class GarnetNetworkInterface(ClockedObject):
    type = 'GarnetNetworkInterface'
    cxx_class = 'NetworkInterface'
    cxx_header = "mem/ruby/network/garnet2.0/NetworkInterface.hh"

    id = Param.UInt32("ID in relation to other network interfaces")
    vcs_per_vnet = Param.UInt32(Parent.vcs_per_vnet,
                             "virtual channels per virtual network")
    virt_nets = Param.UInt32(Parent.number_of_virtual_networks,
                          "number of virtual networks")
    garnet_deadlock_threshold = Param.UInt32(Parent.garnet_deadlock_threshold,
                                      "network-level deadlock threshold")

class GarnetRouter(BasicRouter):
    type = 'GarnetRouter'
    cxx_class = 'Router'
    cxx_header = "mem/ruby/network/garnet2.0/Router.hh"
    vcs_per_vnet = Param.UInt32(Parent.vcs_per_vnet,
                              "virtual channels per virtual network")
    virt_nets = Param.UInt32(Parent.number_of_virtual_networks,
                          "number of virtual networks")
    width = Param.UInt32(Parent.ni_flit_size,
                          "bit width supported by the router")
