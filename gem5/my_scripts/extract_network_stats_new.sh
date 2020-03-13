echo > $1/network_stats.txt
grep "packets_injected::total" $1/stats.txt | sed 's/system.ruby.network.packets_injected::total\s*/packets_injected = /' >> $1/network_stats.txt
grep "packets_received::total" $1/stats.txt | sed 's/system.ruby.network.packets_received::total\s*/packets_received = /' >> $1/network_stats.txt
grep "average_packet_queueing_latency" $1/stats.txt | sed 's/system.ruby.network.average_packet_queueing_latency\s*/average_packet_queueing_latency = /' >> $1/network_stats.txt
grep "average_packet_network_latency" $1/stats.txt | sed 's/system.ruby.network.average_packet_network_latency\s*/average_packet_network_latency = /' >> $1/network_stats.txt
grep "average_packet_latency" $1/stats.txt | sed 's/system.ruby.network.average_packet_latency\s*/average_packet_latency = /' >> $1/network_stats.txt
grep "flits_injected::total" $1/stats.txt | sed 's/system.ruby.network.flits_injected::total\s*/flits_injected = /' >> $1/network_stats.txt
grep "flits_received::total" $1/stats.txt | sed 's/system.ruby.network.flits_received::total\s*/flits_received = /' >> $1/network_stats.txt
grep "average_flit_queueing_latency" $1/stats.txt | sed 's/system.ruby.network.average_flit_queueing_latency\s*/average_flit_queueing_latency = /' >> $1/network_stats.txt
grep "average_flit_network_latency" $1/stats.txt | sed 's/system.ruby.network.average_flit_network_latency\s*/average_flit_network_latency = /' >> $1/network_stats.txt
grep "average_flit_latency" $1/stats.txt | sed 's/system.ruby.network.average_flit_latency\s*/average_flit_latency = /' >> $1/network_stats.txt
grep "average_hops" $1/stats.txt | sed 's/system.ruby.network.average_hops\s*/average_hops = /' >> $1/network_stats.txt
grep "int_link_utilization" $1/stats.txt | sed 's/system.ruby.network.int_link_utilization\s*/int_link_utilization = /' >> $1/network_stats.txt
