/******************************************************************************
Copyright (c) 2020 Georgia Instititue of Technology
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Author : Saeed Rashidi (saeed.rashidi@gatech.edu)
*******************************************************************************/

#ifndef __SYSTEM_HH__
#define __SYSTEM_HH__

#include <map>
#include <math.h>
#include <fstream>
#include <chrono>
#include <ctime>
#include <tuple>
#include <cstdint>
#include <list>
#include <vector>
#include <algorithm>
#include <chrono>
#include <sstream>
#include "CommonAPI.hh"
#define CLOCK_PERIOD 1
#ifndef __BASE_TYPES_HH__
typedef unsigned long long Tick;
#endif
class BaseStream;
class Stream;
class Sys;
class FIFOMovement;
class FIFO;
class BackwardLink;
class DataSet;

enum class ComType {None,Reduce_Scatter, All_Gatehr, All_Reduce, Exchange, Reduction,Forward,All_to_All,All_Reduce_All_to_All};
enum class CollectiveBarrier{Blocking,Non_Blocking};
enum class SchedulingPolicy {LIFO,FIFO,None};
enum class InjectionPolicy {Aggressive,Normal};
enum class PacketRouting {Hardware,Software};
enum class BusType {Both,Shared,Mem};
enum class ParallelismPolicy {MicroBenchmark,Data,Transformer,DLRM};
enum class StreamState {Created,Transferring,Ready,Executing,Zombie,Dead};
enum class EventType {CallEvents,PacketReceived,WaitForVnetTurn,General,TX_DMA,RX_DMA,Wight_Grad_Comm_Finished,Input_Grad_Comm_Finished,Fwd_Comm_Finished,Wight_Grad_Comm_Finished_After_Delay,Input_Grad_Comm_Finished_After_Delay,Fwd_Comm_Finished_After_Delay,Workload_Wait,Reduction_Ready,Rec_Finished,Send_Finished,
    Processing_Finished,Delivered,NPU_to_MA,MA_to_NPU,Read_Port_Free,Write_Port_Free,Apply_Boost,Stream_Transfer_Started,Stream_Ready,Consider_Process,Consider_Retire,Consider_Send_Back,StreamInit,StreamsFinishedIncrease};
enum class ALUState {Free, Forward, WriteBack, Forward_WriteBack, Reduction};


class CallData{
    public:
        ~CallData()= default;
};
class BasicEventHandlerData:public CallData{
public:
    int nodeId;
    EventType event;
    BasicEventHandlerData(int nodeId, EventType event);
    //virtual ~BasicEventHandlerData()= default;
};
class RecvPacketEventHadndlerData:public BasicEventHandlerData,public MetaData{
public:
    BaseStream *owner;
    int vnet;
    int stream_num;
    bool message_end;
    Tick ready_time;
    RecvPacketEventHadndlerData(BaseStream *owner,int nodeId, EventType event,int vnet,int stream_num);
    //~RecvPacketEventHadndlerData()= default;
};
class NetworkStat{
public:
    std::list<double> net_message_latency;
    int net_message_counter;
    NetworkStat(){
        net_message_counter=0;
    }
    void update_network_stat(NetworkStat *networkStat){
        if(net_message_latency.size()<networkStat->net_message_latency.size()){
            int dif=networkStat->net_message_latency.size()-net_message_latency.size();
            for(int i=0;i<dif;i++){
                net_message_latency.push_back(0);
            }
        }
        std::list<double>::iterator it=net_message_latency.begin();
        for(auto &ml:networkStat->net_message_latency){
            (*it)+=ml;
            std::advance(it,1);
        }
        net_message_counter++;
    }
    void take_network_stat_average(){
        for(auto &ml:net_message_latency){
            ml/=net_message_counter;
        }
    }
};
class SharedBusStat:public CallData{
public:
    double total_shared_bus_transfer_queue_delay;
    double total_shared_bus_transfer_delay;
    double total_shared_bus_processing_queue_delay;
    double total_shared_bus_processing_delay;

    double total_mem_bus_transfer_queue_delay;
    double total_mem_bus_transfer_delay;
    double total_mem_bus_processing_queue_delay;
    double total_mem_bus_processing_delay;
    int mem_request_counter;
    int shared_request_counter;
    SharedBusStat(BusType busType,double total_bus_transfer_queue_delay,double total_bus_transfer_delay,
                  double total_bus_processing_queue_delay,double total_bus_processing_delay){
        if(busType==BusType::Shared){
            this->total_shared_bus_transfer_queue_delay=total_bus_transfer_queue_delay;
            this->total_shared_bus_transfer_delay=total_bus_transfer_delay;
            this->total_shared_bus_processing_queue_delay=total_bus_processing_queue_delay;
            this->total_shared_bus_processing_delay=total_bus_processing_delay;

            this->total_mem_bus_transfer_queue_delay=0;
            this->total_mem_bus_transfer_delay=0;
            this->total_mem_bus_processing_queue_delay=0;
            this->total_mem_bus_processing_delay=0;
        }
        else{
            this->total_shared_bus_transfer_queue_delay=0;
            this->total_shared_bus_transfer_delay=0;
            this->total_shared_bus_processing_queue_delay=0;
            this->total_shared_bus_processing_delay=0;

            this->total_mem_bus_transfer_queue_delay=total_bus_transfer_queue_delay;
            this->total_mem_bus_transfer_delay=total_bus_transfer_delay;
            this->total_mem_bus_processing_queue_delay=total_bus_processing_queue_delay;
            this->total_mem_bus_processing_delay=total_bus_processing_delay;
        }
        shared_request_counter=0;
        mem_request_counter=0;
    }
    void update_bus_stats(BusType busType,SharedBusStat *sharedBusStat){
        if(busType==BusType::Shared){
            total_shared_bus_transfer_queue_delay+=sharedBusStat->total_shared_bus_transfer_queue_delay;
            total_shared_bus_transfer_delay+=sharedBusStat->total_shared_bus_transfer_delay;
            total_shared_bus_processing_queue_delay+=sharedBusStat->total_shared_bus_processing_queue_delay;
            total_shared_bus_processing_delay+=sharedBusStat->total_shared_bus_processing_delay;
            shared_request_counter++;
        }
        else if(busType==BusType::Mem){
            total_mem_bus_transfer_queue_delay+=sharedBusStat->total_mem_bus_transfer_queue_delay;
            total_mem_bus_transfer_delay+=sharedBusStat->total_mem_bus_transfer_delay;
            total_mem_bus_processing_queue_delay+=sharedBusStat->total_mem_bus_processing_queue_delay;
            total_mem_bus_processing_delay+=sharedBusStat->total_mem_bus_processing_delay;
            mem_request_counter++;
        }
        else{
            total_shared_bus_transfer_queue_delay+=sharedBusStat->total_shared_bus_transfer_queue_delay;
            total_shared_bus_transfer_delay+=sharedBusStat->total_shared_bus_transfer_delay;
            total_shared_bus_processing_queue_delay+=sharedBusStat->total_shared_bus_processing_queue_delay;
            total_shared_bus_processing_delay+=sharedBusStat->total_shared_bus_processing_delay;
            total_mem_bus_transfer_queue_delay+=sharedBusStat->total_mem_bus_transfer_queue_delay;
            total_mem_bus_transfer_delay+=sharedBusStat->total_mem_bus_transfer_delay;
            total_mem_bus_processing_queue_delay+=sharedBusStat->total_mem_bus_processing_queue_delay;
            total_mem_bus_processing_delay+=sharedBusStat->total_mem_bus_processing_delay;
            shared_request_counter++;
            mem_request_counter++;
        }
    }
    void update_bus_stats(BusType busType,SharedBusStat sharedBusStat){
        if(busType==BusType::Shared){
            total_shared_bus_transfer_queue_delay+=sharedBusStat.total_shared_bus_transfer_queue_delay;
            total_shared_bus_transfer_delay+=sharedBusStat.total_shared_bus_transfer_delay;
            total_shared_bus_processing_queue_delay+=sharedBusStat.total_shared_bus_processing_queue_delay;
            total_shared_bus_processing_delay+=sharedBusStat.total_shared_bus_processing_delay;
            shared_request_counter++;
        }
        else if(busType==BusType::Mem){
            total_mem_bus_transfer_queue_delay+=sharedBusStat.total_mem_bus_transfer_queue_delay;
            total_mem_bus_transfer_delay+=sharedBusStat.total_mem_bus_transfer_delay;
            total_mem_bus_processing_queue_delay+=sharedBusStat.total_mem_bus_processing_queue_delay;
            total_mem_bus_processing_delay+=sharedBusStat.total_mem_bus_processing_delay;
            mem_request_counter++;
        }
        else{
            total_shared_bus_transfer_queue_delay+=sharedBusStat.total_shared_bus_transfer_queue_delay;
            total_shared_bus_transfer_delay+=sharedBusStat.total_shared_bus_transfer_delay;
            total_shared_bus_processing_queue_delay+=sharedBusStat.total_shared_bus_processing_queue_delay;
            total_shared_bus_processing_delay+=sharedBusStat.total_shared_bus_processing_delay;
            total_mem_bus_transfer_queue_delay+=sharedBusStat.total_mem_bus_transfer_queue_delay;
            total_mem_bus_transfer_delay+=sharedBusStat.total_mem_bus_transfer_delay;
            total_mem_bus_processing_queue_delay+=sharedBusStat.total_mem_bus_processing_queue_delay;
            total_mem_bus_processing_delay+=sharedBusStat.total_mem_bus_processing_delay;
            shared_request_counter++;
            mem_request_counter++;
        }
    }
    void take_bus_stats_average(){
        total_shared_bus_transfer_queue_delay/=shared_request_counter;
        total_shared_bus_transfer_delay/=shared_request_counter;
        total_shared_bus_processing_queue_delay/=shared_request_counter;
        total_shared_bus_processing_delay/=shared_request_counter;

        total_mem_bus_transfer_queue_delay/=mem_request_counter;
        total_mem_bus_transfer_delay/=mem_request_counter;
        total_mem_bus_processing_queue_delay/=mem_request_counter;
        total_mem_bus_processing_delay/=mem_request_counter;
    }
};
class StreamStat:public SharedBusStat,public NetworkStat{
public:
    std::list<double > queuing_delay;
    int stream_stat_counter;
    //~StreamStat()= default;
    StreamStat():SharedBusStat(BusType::Shared,0,0,0,0){
        stream_stat_counter=0;
    }
    void update_stream_stats(StreamStat *streamStat){
        update_bus_stats(BusType::Both,streamStat);
        update_network_stat(streamStat);
        if(queuing_delay.size()<streamStat->queuing_delay.size()){
            int dif=streamStat->queuing_delay.size()-queuing_delay.size();
            for(int i=0;i<dif;i++){
                queuing_delay.push_back(0);
            }
        }
        std::list<double>::iterator it=queuing_delay.begin();
        for(auto &tick:streamStat->queuing_delay){
            (*it)+=tick;
            std::advance(it,1);
        }
        stream_stat_counter++;
    }
    void take_stream_stats_average(){
        take_bus_stats_average();
        take_network_stat_average();
        for(auto &tick:queuing_delay){
            tick/=stream_stat_counter;
        }
    }
};
class StatData:public CallData{
public:
    Tick start;
    Tick waiting;
    Tick end;
    //~StatData()= default;
    StatData(){
        start=0;
        waiting=0;
        end=0;
    }
};
class IntData:public CallData{
public:
    int data;
    //~IntData()= default;
    IntData(int d){
        data=d;
    }
};
class Callable{
public:
    virtual ~Callable() = default;
    virtual void call(EventType type,CallData *data)=0;
};
class MyPacket:public Callable
{
public:
    int cycles_needed;
    //FIFOMovement *fMovement;
    FIFO* dest;
    int fm_id;
    int stream_num;
    Callable *notifier;
    Callable *sender;
    int preferred_vnet;
    int preferred_dest;
    int preferred_src;
    Tick ready_time;
    //MyPacket(int cycles_needed, FIFOMovement *fMovement, FIFO *dest);
    MyPacket(int preferred_vnet,int preferred_src ,int preferred_dest);
    void set_notifier(Callable* c);
    void call(EventType event,CallData *data);
    //~MyPacket()= default;
};
class LogGP;
class MemMovRequest:public Callable,public SharedBusStat{
public:
    static int id;
    int my_id;
    int size;
    int latency;
    Callable *callable;
    bool processed;
    bool send_back;
    bool mem_bus_finished;
    Sys *generator;
    EventType callEvent=EventType::General;
    LogGP *loggp;
    std::list<MemMovRequest>::iterator pointer;
    void call(EventType event,CallData *data);

    Tick total_transfer_queue_time;
    Tick total_transfer_time;
    Tick total_processing_queue_time;
    Tick total_processing_time;
    Tick start_time;
    int request_num;
    MemMovRequest(int request_num,Sys *generator,LogGP *loggp,int size,int latency,Callable *callable,bool processed,bool send_back);
    void wait_wait_for_mem_bus(std::list<MemMovRequest>::iterator pointer);
    void set_iterator(std::list<MemMovRequest>::iterator pointer){
        this->pointer=pointer;
    }
    //~MemMovRequest()= default;
};
class DataSet:public Callable,public StreamStat{
public:
    static int id_auto_increment;
    int my_id;
    int total_streams;
    int finished_streams;
    bool finished;
    Tick finish_tick;
    Tick creation_tick;
    std::pair<Callable *,EventType> *notifier;

    DataSet(int total_streams);
    void set_notifier(Callable *layer,EventType event);
    void notify_stream_finished(StreamStat *data);
    void call(EventType event,CallData *data);
    bool is_finished();
    //~DataSet()= default;
};
class MemBus;
class LogGP:public Callable{
public:
    enum class State{Free,waiting,Sending,Receiving};
    enum class ProcState{Free,Processing};
    int request_num;
    std::string name;
    Tick L;
    Tick o;
    Tick g;
    double G;
    Tick last_trans;
    State curState;
    State prevState;
    ProcState processing_state;
    std::list<MemMovRequest> sends;
    std::list<MemMovRequest> receives;
    std::list<MemMovRequest> processing;

    std::list<MemMovRequest> retirements;
    std::list<MemMovRequest> pre_send;
    std::list<MemMovRequest> pre_process;
    std::list<MemMovRequest>::iterator talking_it;

    LogGP *partner;
    Sys *generator;
    EventType trigger_event;
    int subsequent_reads;
    int THRESHOLD;
    int local_reduction_delay;
    LogGP(std::string name,Sys *generator,Tick L,Tick o,Tick g,double G,EventType trigger_event);
    void process_next_read();
    void request_read(int bytes,bool processed,bool send_back,Callable *callable);
    void switch_to_receiver(MemMovRequest mr,Tick offset);
    void call(EventType event,CallData *data);
    MemBus *NPU_MEM;
    void attach_mem_bus(Sys *generator,Tick L,Tick o,Tick g,double G,bool model_shared_bus,int communication_delay);
    ~LogGP();
};
class MemBus{
    public:
        LogGP *NPU_side;
        LogGP *MA_side;
        Sys *generator;
        int communication_delay;
        bool model_shared_bus;
        ~MemBus();
        MemBus(std::string side1,std::string side2,Sys *generator,Tick L,Tick o,Tick g,double G,bool model_shared_bus,int communication_delay,bool attach);
        void send_from_NPU_to_MA(int bytes,bool processed,bool send_back,Callable *callable);
        void send_from_MA_to_NPU(int bytes,bool processed,bool send_back,Callable *callable);
};
class Algorithm;
class LogicalTopology;
class VnetInfo{
    public:
        Sys *generator;
        std::string direction;
        int burst;
        int flits;
        int dest_node;
        int sender_node;
        int parallel_reduce;
        int processing_latency;
        int nodes_in_vnet;
        int communication_delay;
        int packets_per_message;
        int vnet_num;
        int message_size;
        int initial_data_size;
        int final_data_size;
        int packet_size;
        bool enabled;
        ComType comm_type;
        Algorithm *algorithm;
        InjectionPolicy injectionPolicy;
        PacketRouting packetRouting;
        VnetInfo(Sys *generator,std::string direction,int vnet_num,int burst,int flits,int dest_node,int parallel_reduce,int processing_latency, int nodes_in_vnet, int communication_delay,
                 int packets_per_message,int message_size,bool enabled);
        VnetInfo(Sys *generator,int Vnet_num,Algorithm *algorithm);
        VnetInfo();
        void init();
        void init(BaseStream *stream);
        void adjust_message_size(int size);
        int clone_for_all_reduce(int size_of_stream);
        int clone_for_exhange(int size_of_stream);
        int clone_for_reduction(int size_of_stream);
        int clone_for_reduce_scatter(int size_of_stream);
        int clone_for_all_to_all_on_torus(int size_of_stream,PacketRouting rounting,InjectionPolicy ip);
        int clone_for_all_to_all_on_alltoall(int size_of_stream);
        int clone_for_all_reduce_single_on_alltoall(int size_of_stream);
        int clone_for_all_reduce_single_on_torus(int size_of_stream);
        //int clone_for_all_reduce_single_on_tree(int size_of_stream);
        int clone_for_reduce_scatter_single_on_alltoall(int size_of_stream);
        int clone_for_reduce_scatter_single_on_torus(int size_of_stream);
        int clone_for_all_gather(int size_of_stream);
        int clone_for_all_gather_single_on_alltoall(int size_of_stream);
        int clone_for_all_gather_single_on_torus(int size_of_stream);
        void makeSinglePacketFLit();
};
class DMA_Request{
public:
    int id;
    int slots;
    int latency;
    bool executed;
    int bytes;
    Callable *stream_owner;
    DMA_Request(int id,int slots,int latency,int bytes);
    DMA_Request(int id,int slots,int latency,int bytes,Callable *stream_owner);
};
/*class MessageSlot
{
public:
    int PACKET_SLOTS;
    std::vector<int> filled;
    MessageSlot(int slots);
    bool write_packet(int stream_num);
    bool is_accessible(int stream_num);
    bool remove_packet(int stream_num);
};
class FIFO:public Callable
{
public:
    int id;
    int SIZE;
    int PACKETS_PER_SLOT;
    int DMA_SLOT_LATENCY;
    int ACCESS_LATENCY;
    bool has_TX_DMA;
    bool has_express_out;
    bool has_RX_DMA;
    bool read_port_free;
    bool write_port_free;
    int PACKET_SIZE;
    Sys *generator;
    Tick last_read;
    std::list<MessageSlot> filled;
    std::list<DMA_Request> TX_Requests;

    std::list<std::pair<int,int>> reserve_requests;
    int reserved;

    std::list<DMA_Request> RX_Requests;

    int reserved_packets;
    int total_free_packet_slots;
    virtual void request_reserve(int stream_id,int packets);
    int get_reserve_owner();
    virtual bool is_allowed_to_write(int id);
    void remove_head_reserve_request();
    bool ask_for_packet_write(int id);
    bool is_free_to_write();
    virtual bool write_packet(int stream_num);
    virtual bool is_accessible(int stream_num);
    virtual bool remove_packet(int stream_num);

    FIFO(Sys *generator,int id,int SIZE,int PACKET_SIZE,int PACKETS_PER_SLOT,int DMA_SLOT_LATENCY, bool has_TX_DMA,bool has_express_out,bool has_RX_DMA);
    virtual void request_TX_DMA(int id,Callable *callable,int bytes);
    void request_RX_DMA(int id,Callable *stream,int bytes);
    virtual bool has_space(int slots);
    virtual bool has_packet_space();
    virtual bool acquire_packet_space();
    virtual void instant_write(int streamm_num,int bytes);
    virtual void instant_read(int streamm_num,int bytes);
    virtual void process_TX_request(SharedBusStat *mdata);
    virtual void process_RX_request(SharedBusStat *mdata);
    void call(EventType event,CallData *data);
};
class RAM:public FIFO{
public:
    std::map<int,int> ram;
    int TOTAL_PACKETS;
    RAM(Sys *generator,int id,int SIZE,int PACKET_SIZE,int PACKETS_PER_SLOT,int DMA_SLOT_LATENCY,
        bool has_TX_DMA,bool has_express_out,bool has_RX_DMA);
    bool is_allowed_to_write(int id);
    bool acquire_packet_space();
    void request_reserve(int stream_id,int packets);
    bool write_packet(int stream_num);
    bool is_accessible(int stream_num);
    bool remove_packet(int stream_num);
    void request_TX_DMA(int id,Callable *callable,int bytes);
    bool has_packet_space();
    bool has_space(int slots);
    void instant_write(int streamm_num,int bytes);
    void instant_read(int streamm_num,int bytes);
    void process_TX_request(SharedBusStat *mdata);
    void process_RX_request(SharedBusStat *mdata);
};
class FIFOReduction
{
public:
    std::list<FIFOMovement*> src;
    FIFOMovement *dst;
    int packets;
    int st_num;
    int my_vnet;
    std::list<int> vnets_to_check;
    std::list<FIFOMovement*>::iterator index;
    FIFOReduction(std::list<FIFOMovement*> src,FIFOMovement *dst,int packets,int st_num,
                  int my_vnet, std::list<int> vnets_to_check);
};
class ALU:public Callable
{
public:
    //bool busy;
    int id;
    int remaining;
    int LATENCY;
    int forward_vnet;
    ALUState current_State;
    MyPacket *owner;
    NetworkInterface *ni;
    Sys *generator;
    Stream *my_stream;
    std::list<FIFOReduction> reductions;
    Tick local_timeStamp;

    static Tick global_utilization;
    static int total_ALUs;

    static void report();
    //void register
    ALU(int id,Sys *generator,int LATENCY);
    bool is_busy();
    bool acquire_for_reduction(std::list<FIFOMovement*> src,FIFOMovement *dst,int packets,int st_num,
                               int my_vnet,std::list<int> vnets_to_check);
    bool acquire_for_forward(MyPacket *owner,int vnet);
    bool acquire_for_writeback(MyPacket *owner);
    bool acquire_for_forward_writeback(MyPacket *owner,int vnet);
    void call(EventType event,CallData *data);
};
class BackwardLink:public Callable
{
public:
    int id;
    bool busy;
    //int remaining;
    int LATENCY;
    FIFO* dest;
    int stream_num;
    MyPacket* owner;
    bool del;
    Tick local_timeStamp;
    Sys *generator;
    static Tick global_utilization;
    static int total_links;

    static void report();
    BackwardLink(Sys* generator,int id,int LATENCY);
    bool acquire(MyPacket *owner);
    bool is_busy();
    bool acquire_and_delete(MyPacket *owner);
    void call(EventType event,CallData *data);


};
class FIFOMovement
{
public:
    int id;
    FIFO *source;
    FIFO *current_dest;
    std::vector<FIFO*> destinations;
    int dest_pointer;
    int read_burst;
    int change_read;
    //int receive_burst;
    int nodes_in_vnet;
    int input_packets;
    ComType type;
    Sys *owner;
    int stream_num;
    //int BURSTMAX;
    int accepted_messages;
    ALU *tempALU;
    BackwardLink *tempLink;
    Stream *my_stream;
    int packets_per_message;
    bool initialized;
    bool report;
    int test;
    int test2;
    int all_to_all_packets_to_receive;
    FIFOMovement(int id,Stream *my_stream,FIFO *source,std::vector<FIFO*> destinations);
    void init() ;
    FIFO* get_next_dest();
    void process_next_read_burst();
    void process_next_read_burst(int reduction);
    bool is_finished();
    bool ready(int vnet);
    std::list<int> vnets_to_find;

};*/
class BaseStream:public Callable,public  StreamStat{
public:
    static std::map<int,int> synchronizer;
    static std::map<int,int> ready_counter;
    static std::map<int,std::list<BaseStream *>> suspended_streams;
    virtual ~BaseStream()=default;
    int stream_num;
    int stream_count;
    int total_packets_sent;
    int packet_num;
    SchedulingPolicy preferred_scheduling;
    //std::vector<ComType> com_types;
    std::list<VnetInfo> vnets_to_go;
    //std::vector<int> latency_count;
    int current_vnet;
    VnetInfo my_info;
    //int current_latency_count;
    ComType current_com_type;
    Tick creation_time;
    Sys *owner;
    DataSet *dataset;
    int steps_finished;
    int initial_data_size;
    int not_delivered;
    StreamState state;

    Tick last_vnet_change;
    bool data_collected;

    int test;
    int test2;
    uint64_t phase_latencies[10];

    void changeState(StreamState state);
    virtual void consume(RecvPacketEventHadndlerData *message)=0;
    virtual void init()=0;
    virtual void search_for_early_packets();

    BaseStream(int stream_num,Sys *owner,std::list<VnetInfo> vnets_to_go);
    void declare_ready();
    bool is_ready();
    void consume_ready();
    void suspend_ready();
    void resume_ready(int st_num);
    void destruct_ready();

};
class PacketBundle:public Callable{
public:
    std::list<MyPacket*> locked_packets;
    bool processed;
    bool send_back;
    int size;
    Sys *generator;
    BaseStream *stream;
    Tick creation_time;
    PacketBundle(Sys *generator,BaseStream *stream,std::list<MyPacket*> locked_packets, bool processed, bool send_back, int size);
    PacketBundle(Sys *generator,BaseStream *stream, bool processed, bool send_back, int size);
    void send_to_MA();
    void send_to_NPU();
    void call(EventType event,CallData *data);
    //~PacketBundle()= default;
};
class StreamBaseline: public BaseStream
{
public:

    int max_count;
    int zero_latency_packets;
    int non_zero_latency_packets;
    std::list<MyPacket> packets;
    bool finished_packet_received;
    int switch_delay;
    Tick final_compute;
    std::map<int,int> alltoall_synchronizer;
    bool initialized;
    bool toggle;
    //Callable *communicator;
    //bool in_progress;
    /*~StreamBaseline(){
        if(owner->id==0){
            std::map<int,int>::iterator it;
            it=synchronizer.find(stream_num);
            synchronizer.erase(it);
            ready_counter.erase(stream_num);
        }
    }*/
    int remained_packets_per_message;
    int remained_packets_per_max_count;

    std::list<MyPacket*> locked_packets;
    long free_packets;

    bool processed;
    bool send_back;
    bool NPU_to_MA;

    StreamBaseline(Sys *owner,DataSet *dataset,int stream_num,std::list<VnetInfo> vnets_to_go);
    void init();
    void call(EventType event,CallData *data);
    void consume(RecvPacketEventHadndlerData *message);
    //~StreamBaseline()= default;
};

/*class Stream:public BaseStream
{
public:

    ~Stream(){
        synchronizer[stream_num]--;
        if(synchronizer[stream_num]==0){
            std::map<int,int>::iterator it;
            it=synchronizer.find(stream_num);
            synchronizer.erase(it);
            ready_counter.erase(stream_num);
        }
    }
    //variables for the proposed architecture
    std::vector<FIFO*> FIFOs_to_go;
    std::list<MyPacket*> myPackets;
    std::list<FIFOMovement> FIFO_movements;
    std::list<FIFOMovement> finished_FIFO_movements;
    int remained_packets_from_message;


    void search_for_early_packets();
    void process_next_dest();
    Stream(Sys *owner,DataSet *dataset,int stream_num,std::list<VnetInfo> vnets_to_go);
    void call(EventType event,CallData *data);
    void reduce();
    void accept_packet(MyPacket *packet);
    bool iteratable();
    void consume_only(RecvPacketEventHadndlerData *message);
    void consume(RecvPacketEventHadndlerData *message);
    bool ready();
};*/

#include "Workload.hh"
class VnetLevels;
class Sys:public Callable
{
public:
    class SchedulerUnit{
        public:
            Sys *sys;
            int ready_list_threshold;
            int queue_threshold;
            std::map<int,int> running_streams;
            std::map<int,std::list<BaseStream*>::iterator> stream_pointer;
            SchedulerUnit(Sys *sys,std::vector<int> queues,int ready_list_threshold,int queue_threshold);
            void notify_stream_removed(int vnet);
            void notify_stream_added(int vnet);
            void notify_stream_added_into_ready_list();
    };
    SchedulerUnit *scheduler_unit;
    enum class CollectiveOptimization{Baseline,LocalBWAware};
    enum class CollectiveImplementation{AllToAll,HierarchicalRing,DoubleBinaryTree};
    ~Sys();
    CommonAPI *NI;
    int finished_workloads;
    int id;

    CollectiveImplementation collectiveImplementation;
    CollectiveOptimization collectiveOptimization;

    std::vector<int> local_vnets;
    std::vector<int> vertical_vnets1;
    std::vector<int> vertical_vnets2;
    std::vector<int> horizontal_vnets1;
    std::vector<int> horizontal_vnets2;
    std::vector<int> perpendicular_vnets1;
    std::vector<int> perpendicular_vnets2;
    std::vector<int> fourth_vnets1;
    std::vector<int> fourth_vnets2;

    std::chrono::high_resolution_clock::time_point start_sim_time;
    std::chrono::high_resolution_clock::time_point end_sim_time;

    std::list<Callable *> registered_for_finished_stream_event;

    int local_dim;
    int vertical_dim;
    int horizontal_dim;
    int perpendicular_dim;
    int fourth_dim;
    int local_queus;
    int vertical_queues;
    int horizontal_queues;int perpendicular_queues;
    int fourth_queues;
    bool boost_mode;

    int processing_latency;
    int communication_delay;

    int preferred_dataset_splits;
    PacketRouting alltoall_routing;
    InjectionPolicy alltoall_injection_policy;
    float compute_scale;
    float comm_scale;
    int local_reduction_delay;
    std::map<int,int> myAllToAllVnet;
    std::map<int,int> myAllToAlldest;
    uint64_t pending_events;

    int workload_event_index;
    std::string method;
    //for test
    Workload *workload;
    MemBus *memBus;
    int all_vnets;
    std::map<int,std::vector<RecvPacketEventHadndlerData*>> earlyPackets;
    //for supporting LIFO
    std::list<BaseStream*> ready_list;
    SchedulingPolicy scheduling_policy;
    int first_phase_streams;
    std::map<int,std::list<BaseStream*>> active_Streams;
    std::map<int,std::list<int>> stream_priorities;
    std::map<int,VnetInfo*> vnet_info;
    //std::map<int,std::vector<FIFO>> FIFOs;
    //std::vector<RAM> RAMs;
    std::map<int,int> FIFO_allocator;
    int RAM_allocator;
    //std::vector<ALU> ALUs;
    //std::vector<BackwardLink> BackwardLinks;
    VnetLevels *vLevels;
    std::map<std::string,LogicalTopology*> logical_topologies;
    std::map<Tick,std::list<std::tuple<Callable*,EventType,CallData *>>> event_queue;
    //int event_queue_size;
    static std::vector<std::vector<Sys*>> masked_generators;
    std::map<int,int> stream_change_history;
    static int total_nodes;
    static Tick offset;
    static Tick increase;
    static Tick last_tick;
    static Tick min_boost;
    static int requester_counter;
    static std::vector<Sys*> all_generators;
    static uint8_t *dummy_data;
    //for reports
    uint64_t streams_injected;
    uint64_t streams_finished;
    static uint64_t streams_time;
    static uint64_t phase_latencies[10];
    int local_allocator_first;
    int local_allocator_last;
    int vertical_allocator;
    int horizontal_allocator;
    int perpendicular_allocator;
    int fourth_allocator;
    int stream_counter;
    int ALU_reduction_allocator;
    bool enabled;


    std::string inp_scheduling_policy;
    std::string inp_packet_routing;
    std::string inp_injection_policy;
    std::string inp_collective_implementation;
    std::string inp_collective_optimization;
    float inp_L;
    float inp_o;
    float inp_g;
    float inp_G;
    int inp_model_shared_bus;
    bool model_shared_bus;
    int inp_boost_mode;


    int total_FIFO_size;
    int RAM_size;

    void register_for_finished_stream(Callable *callable);
    void increase_finished_streams(int amount);
    void register_event(Callable *callable,EventType event,CallData *callData,int cycles);
    void insert_into_ready_list(BaseStream *stream);
    void ask_for_schedule();
    void schedule(int num);
    //ALU *get_next_ALU();
    //BackwardLink *get_next_BackwardLink();
    //for boosting simulation speed
    void register_vnets(BaseStream *stream,std::list<VnetInfo> vnets_to_go);
    void call(EventType type,CallData *data);
    void try_register_event(Callable *callable,EventType event,CallData *callData,Tick &cycles);
    void apply_boost(Tick boost);
    void call_events();
    void workload_finished(){finished_workloads++;};
    void register_stream_change(int st_num);
    void check_stream_change_history(BaseStream *stream);
    static Tick boostedTick();
    static void ask_for_boost(Tick boost_amount);
    int get_next_node(int cuurent_node,int vnet_num);
    int get_next_sender_node(int cuurent_node,int vnet_num);
    static void notify_stream_change(int stream_num,int vnet,std::string meth);
    static void exiting();
    int nextPowerOf2(int n);
    int get_next_local_vnet();
    int get_next_local_vnet_first();
    int get_next_local_vnet_last();
    int get_next_vertical_vnet();
    int get_next_horizontal_vnet();
    int get_next_horizontal_vnet_alltoall();
    int get_next_perpendicular_vnet();
    int get_next_fourth_vnet();
    static void sys_panic(std::string msg);
    void exitSimLoop(std::string msg);

    Sys(CommonAPI *NI,int id,int num_passes,int local_dim, int vertical_dim,int horizontal_dim,
             int perpendicular_dim,int fourth_dim,int local_queus,int vertical_queues,int horizontal_queues,
             int perpendicular_queues,int fourth_queues,std::string my_sys,
             std::string my_workload,float comm_scale,float compute_scale,int total_stat_rows,int stat_row, std::string path,std::string run_name);

    void iterate();
    void initialize_sys(std::string name);
    void parse_var(std::string var,std::string value);
    void post_process_inputs();
    DataSet * generate_all_reduce(int size,bool local, bool vertical, bool horizontal,SchedulingPolicy pref_scheduling);
    DataSet * generate_all_to_all(int size,bool local, bool vertical, bool horizontal,SchedulingPolicy pref_scheduling);
    DataSet * generate_all_gather(int size,bool local, bool vertical, bool horizontal,SchedulingPolicy pref_scheduling);
    DataSet *generate_alltoall_all_to_all(int size);
    DataSet *generate_alltoall_all_reduce(int size);
    DataSet *generate_tree_all_reduce(int size);
    DataSet *generate_hierarchichal_all_to_all(int size,bool local_run,bool vertical_run, bool horizontal_run, SchedulingPolicy pref_scheduling);
    DataSet *generate_hierarchichal_all_reduce(int size,bool local_run, bool vertical_run, bool horizontal_run, SchedulingPolicy pref_scheduling);
    DataSet *generate_hierarchichal_all_gather(int size,bool local_run, bool vertical_run, bool horizontal_run, SchedulingPolicy pref_scheduling);
    bool search_for_active_streams(int vnet_to_search,int dest,int dest_vnet);
    bool ready(int vnet);
    void consume(RecvPacketEventHadndlerData *message);
    void next_vnet(BaseStream* stream);
    //void check_for_global_event_insertion(int cycles,EventType event);
    void proceed_to_next_vnet_baseline(StreamBaseline *stream);
    //void proceed_to_next_vnet(Stream *stream);
    static void handleEvent(void *arg);
    timespec_t generate_time(int cycles);
};
class ComputeNode{

};
class Node:public ComputeNode{
    public:
        int id;
        Node *parent;
        Node *left_child;
        Node *right_child;
        Node(int id,Node *parent,Node *left_child,Node *right_child);
};
class LogicalTopology{
    public:
        virtual LogicalTopology* get_topology();
        static int get_reminder(int number,int divisible);
        virtual ~LogicalTopology()= default;
};
class Torus:public LogicalTopology{
    public:
        enum Dimension{Local,Vertical,Horizontal,Perpendicular,Fourth};
        enum Direction{Clockwise,Anticlockwise};
        int id;
        int local_node_id;
        int right_node_id;
        int left_node_id;
        int up_node_id;
        int down_node_id;
        int Zpositive_node_id,Znegative_node_id,Fpositive_node_id,Fnegative_node_id;
        int total_nodes,local_dim,horizontal_dim,vertical_dim,perpendicular_dim;
        Torus(int id,int total_nodes,int local_dim,int horizontal_dim,int vertical_dim,int perpendicular_dim);
        void find_neighbors(int id,int total_nodes,int local_dim,int horizontal_dim,int vertical_dim,int perpendicular_dim);
        virtual int get_receiver_node(int id,Dimension dimension,Direction direction);
        virtual int get_sender_node(int id,Dimension dimension,Direction direction);
        int get_nodes_in_ring(Dimension dimension);
        bool is_enabled(int id,Dimension dimension);
};
class BinaryTree:public Torus{
    public:
        enum TreeType{RootMax,RootMin};
        enum Type{Leaf,Root,Intermediate};
        int total_tree_nodes;
        int start;
        TreeType tree_type;
        int stride;
        Node *tree;
        virtual ~BinaryTree();
        std::map<int,Node*> node_list;
        BinaryTree(int id,TreeType tree_type,int total_tree_nodes,int start,int stride,int local_dim);
        Node* initialize_tree(int depth,Node* parent);
        void build_tree(Node* node);
        int get_parent_id(int id);
        int get_left_child_id(int id);
        int get_right_child_id(int id);
        Type get_node_type(int id);
        void print(Node *node);
        virtual int get_receiver_node(int id,Dimension dimension,Direction direction);
        virtual int get_sender_node(int id,Dimension dimension,Direction direction);

};
class DoubleBinaryTreeTopology:public LogicalTopology{
    public:
        int counter;
        BinaryTree *DBMAX;
        BinaryTree *DBMIN;
        LogicalTopology *get_topology();
        ~DoubleBinaryTreeTopology();
        DoubleBinaryTreeTopology(int id,int total_tree_nodes,int start,int stride,int local_dim);
};
class AllToAllTopology:public Torus{
    public:
        AllToAllTopology(int id,int total_nodes,int local_dim,int alltoall_dim);
        int get_receiver_node(int id,Dimension dimension,Direction direction);
        int get_sender_node(int id,Dimension dimension,Direction direction);
};
class Algorithm:public Callable{
    public:
        enum Name{Ring,DoubleBinaryTree,AllToAll};
        Name name;
        int id;
        BaseStream *stream;
        LogicalTopology *logicalTopology;
        int data_size;
        int final_data_size;
        ComType comType;
        bool enabled;
        Algorithm();
        virtual ~Algorithm()= default;
        virtual void run(EventType event,CallData *data)=0;
        virtual void exit();
        virtual void init(BaseStream *stream);
        virtual void call(EventType event,CallData *data);
};
class Ring:public Algorithm{
    public:
        enum Type{AllReduce,AllGather,ReduceScatter,AllToAll};
        Torus::Dimension dimension;
        Torus::Direction direction;
        int zero_latency_packets;
        int non_zero_latency_packets;
        int id;
        int current_receiver;
        int current_sender;
        int nodes_in_ring;
        Type type;
        int stream_count;
        int max_count;
        int remained_packets_per_max_count;
        int remained_packets_per_message;
        int parallel_reduce;
        PacketRouting routing;
        InjectionPolicy injection_policy;
        std::list<MyPacket> packets;
        bool toggle;
        long free_packets;
        long total_packets_sent;
        int msg_size;

        std::list<MyPacket*> locked_packets;
        bool processed;
        bool send_back;
        bool NPU_to_MA;

        Ring(Type type,int id,Torus *torus,int data_size,Torus::Dimension dimension, Torus::Direction direction,PacketRouting routing,InjectionPolicy injection_policy,bool boost_mode);
        void run(EventType event,CallData *data);
        void process_stream_count();
        //void call(EventType event,CallData *data);
        void release_packets();
        virtual void process_max_count();
        void reduce();
        bool iteratable();
        virtual int get_non_zero_latency_packets();
        void insert_packet(Callable *sender);
        bool ready();

        void exit();
};
class DoubleBinaryTreeAllReduce:public Algorithm{
    public:
        enum State{Begin,WaitingForTwoChildData,WaitingForOneChildData,SendingDataToParent,WaitingDataFromParent,SendingDataToChilds,End};
        void run(EventType event,CallData *data);
        //void call(EventType event,CallData *data);
        //void exit();
        int parent;
        int left_child;
        int reductions;
        int right_child;
        //BinaryTree *tree;
        BinaryTree::Type type;
        State state;
        DoubleBinaryTreeAllReduce(int id,BinaryTree *tree,int data_size,bool boost_mode);
        //void init(BaseStream *stream);
};
class AllToAll:public Ring{
    public:
        AllToAll(Type type,int id,AllToAllTopology *allToAllTopology,int data_size,Torus::Dimension dimension, Torus::Direction direction,PacketRouting routing,InjectionPolicy injection_policy,bool boost_mode);
        void process_max_count();
        int get_non_zero_latency_packets();
};
class VnetLevelHandler{
    public:
        std::vector<int> vnets;
        int allocator;
        int first_allocator;
        int last_allocator;
        int level;
        VnetLevelHandler(int level,int start,int end);
        std::pair<int,Torus::Direction> get_next_vnet_number();
        std::pair<int,Torus::Direction> get_next_vnet_number_first();
        std::pair<int,Torus::Direction> get_next_vnet_number_last();
};
class VnetLevels{
    public:
        std::vector<VnetLevelHandler> levels;
        std::pair<int,Torus::Direction> get_next_vnet_at_level(int level);
        std::pair<int,Torus::Direction> get_next_vnet_at_level_first(int level);
        std::pair<int,Torus::Direction> get_next_vnet_at_level_last(int level);
        VnetLevels(int levels, int vnets_per_level,int offset);
        VnetLevels(std::vector<int> lv,int offset);
};

#endif