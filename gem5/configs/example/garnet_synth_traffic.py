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
# Author: Tushar Krishna

from __future__ import print_function

import m5
from m5.objects import *
from m5.defines import buildEnv
from m5.util import addToPath
import os, optparse, sys

addToPath('../')

from common import Options
from ruby import Ruby
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
# Get paths we might need.  It's expected this file is in m5/configs/example.
config_path = os.path.dirname(os.path.abspath(__file__))
config_root = os.path.dirname(config_path)
m5_root = os.path.dirname(config_root)

parser = optparse.OptionParser()
Options.addNoISAOptions(parser)

parser.add_option("--synthetic", type="choice", default="uniform_random",
                  choices=['uniform_random', 'tornado', 'bit_complement', \
                           'bit_reverse', 'bit_rotation', 'neighbor', \
                            'shuffle', 'transpose','training'])

parser.add_option("-i", "--injectionrate", type="float", default=0.1,
                  metavar="I",
                  help="Injection rate in packets per cycle per node. \
                        Takes decimal value between 0 to 1 (eg. 0.225). \
                        Number of digits after 0 depends upon --precision.")

parser.add_option("--precision", type="int", default=3,
                  help="Number of digits of precision after decimal point\
                        for injection rate")

parser.add_option("--sim-cycles", type="int", default=1000,
                   help="Number of simulation cycles")

parser.add_option("--num-packets-max", type="int", default=-1,
                  help="Stop injecting after --num-packets-max.\
                        Set to -1 to disable.")

parser.add_option("--single-sender-id", type="int", default=-1,
                  help="Only inject from this sender.\
                        Set to -1 to disable.")

parser.add_option("--single-dest-id", type="int", default=-1,
                  help="Only send to this destination.\
                        Set to -1 to disable.") 

parser.add_option("--inj-vnet", type="int", default=-1,
                  help="Only inject in this vnet (0, 1 or 2).\
                        0 and 1 are 1-flit, 2 is 5-flit.\
                        Set to -1 to inject randomly in all vnets.")

#
# Add the ruby specific and protocol specific options
#
Ruby.define_options(parser)

execfile(os.path.join(config_root, "common", "Options.py"))

(options, args) = parser.parse_args()

if args:
     print("Error: script doesn't take any positional arguments")
     sys.exit(1)


if options.inj_vnet > 2:
    print("Error: Injection vnet %d should be 0 (1-flit), 1 (1-flit) "
          "or 2 (5-flit) or -1 (random)" % (options.inj_vnet))
    sys.exit(1)

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

cpus = [ GarnetSyntheticTraffic(
                     num_packets_max=options.num_packets_max,
                     single_sender=options.single_sender_id,
                     single_dest=options.single_dest_id,
                     sim_cycles=options.sim_cycles,
                     traffic_type=options.synthetic,
                     inj_rate=options.injectionrate,
                     inj_vnet=options.inj_vnet,
                     precision=options.precision,
                     burst_length=options.local_burst_length,
                     burst_interval=options.burst_interval,
                     num_packages=options.num_packages,
                     num_dest=options.num_dirs) \
         for i in xrange(options.num_cpus) ]

# create the desired simulated system
system = System(cpu = cpus, mem_ranges = [AddrRange(options.mem_size)])


# Create a top-level voltage domain and clock domain
system.voltage_domain = VoltageDomain(voltage = options.sys_voltage)

system.clk_domain = SrcClockDomain(clock = options.sys_clock,
                                   voltage_domain = system.voltage_domain)

Ruby.create_system(options, False, system)

# Create a seperate clock domain for Ruby
system.ruby.clk_domain = SrcClockDomain(clock = options.ruby_clock,
                                        voltage_domain = system.voltage_domain)

i = 0
for ruby_port in system.ruby._cpu_ports:
     #
     # Tie the cpu test ports to the ruby cpu port
     #
     cpus[i].test = ruby_port.slave
     i += 1

# -----------------------
# run simulation
# -----------------------

root = Root(full_system = False, system = system)
root.system.mem_mode = 'timing'

# Not much point in this being higher than the L1 latency
m5.ticks.setGlobalFrequency('1ns')

# instantiate configuration
m5.instantiate()

# simulate until program terminates
exit_event = m5.simulate(options.abs_max_tick)

print('Exiting @ tick', m5.curTick(), 'because', exit_event.getCause())
