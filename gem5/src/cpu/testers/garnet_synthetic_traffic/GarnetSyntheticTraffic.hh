/*
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
 * Authors: Tushar Krishna
 */

#ifndef __CPU_GARNET_SYNTHETIC_TRAFFIC_HH__
#define __CPU_GARNET_SYNTHETIC_TRAFFIC_HH__

#include <set>

#include "base/statistics.hh"
#include "mem/mem_object.hh"
#include "mem/port.hh"
#include "params/GarnetSyntheticTraffic.hh"
#include "sim/eventq.hh"
#include "sim/sim_exit.hh"
#include "sim/sim_object.hh"
#include "sim/stats.hh"

enum TrafficType {BIT_COMPLEMENT_ = 0,
                  BIT_REVERSE_ = 1,
                  BIT_ROTATION_ = 2,
                  NEIGHBOR_ = 3,
                  SHUFFLE_ = 4,
                  TORNADO_ = 5,
                  TRANSPOSE_ = 6,
                  UNIFORM_RANDOM_ = 7,
				  TRAINING_ = 8,
                  NUM_TRAFFIC_PATTERNS_};

class Packet;
class GarnetSyntheticTraffic : public MemObject
{
  public:
    typedef GarnetSyntheticTrafficParams Params;
    GarnetSyntheticTraffic(const Params *p);
	
	//mycode
	//static bool isItTraining(){return globalTraffic==UNIFORM_RANDOM_;};
	
    virtual void init();

    // main simulation loop (one cycle)
    void tick();

    virtual BaseMasterPort &getMasterPort(const std::string &if_name,
                                          PortID idx = InvalidPortID);

    /**
     * Print state of address in memory system via PrintReq (for
     * debugging).
     */
    void printAddr(Addr a);
    
    //mycode
    /*class Stream{
        public:
            Stream(int st,int sn, int pl){stream_type=st;stream_num=sn; packets_left=pl;};
            int stream_num;
            int packets_left;
            unsigned char stream_type;
    };*/
    //void phaseOneEnded(unsigned long st_num);
    //void phaseTwoEnded(unsigned long st_num);
    //int pickFromHorizontals();
    //int pickFromVerticals();
    int test;
    //int test2;

  protected:
    EventFunctionWrapper tickEvent;

    class CpuPort : public MasterPort
    {
        GarnetSyntheticTraffic *tester;

      public:

        CpuPort(const std::string &_name, GarnetSyntheticTraffic *_tester)
            : MasterPort(_name, _tester), tester(_tester)
        { }

      protected:

        virtual bool recvTimingResp(PacketPtr pkt);

        virtual void recvReqRetry();
    };

    CpuPort cachePort;

    class GarnetSyntheticTrafficSenderState : public Packet::SenderState
    {
      public:
        /** Constructor. */
        GarnetSyntheticTrafficSenderState(uint8_t *_data)
            : data(_data)
        { }

        // Hold onto data pointer
        uint8_t *data;
    };

    PacketPtr retryPkt;
    unsigned size;
    int id;

    std::map<std::string, TrafficType> trafficStringToEnum;

    unsigned blockSizeBits;

    Tick noResponseCycles;

    int numDestinations;
    Tick simCycles;
    int numPacketsMax;
    int numPacketsSent;
    int singleSender;
    int singleDest;

    std::string trafficType; // string
    TrafficType traffic; // enum from string
    double injRate;
    int injVnet;
    int precision;

    const Cycles responseLimit;

    MasterID masterId;
	
	//mycode
    const int num_packages;
	const int burst_length;
	const int burst_interval;
    const int vertical_burst_length;
	//std::vector<Stream> horizontals;
    //std::vector<Stream> verticals;
    //int stream_counter;
    //int stream_num;
    //int stream_start;
    //int stream_end;
    //unsigned short stream_type;
    //int packet_vnet;
    
    //int counter;
	//bool isBurst;
    
	//static TrafficType globalTraffic;

    void completeRequest(PacketPtr pkt);

    void generatePkt();
    void sendPkt(PacketPtr pkt);
    void initTrafficType();

    void doRetry();

    friend class MemCompleteEvent;
};

#endif // __CPU_GARNET_SYNTHETIC_TRAFFIC_HH__
