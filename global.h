#pragma once
#include <systemc.h>

#define TRACE 1

#define MAX_NUM_PROCESSORS 16

// Transaction  Commands (function h())
#define LOAD  0
#define STORE 1
#define TX_BEGIN 2
#define TX_END 3

// Directory States (function f())
#define   M 1
#define   S 2
#define   I 3

// TVC States (function m())
#define   M 1
#define   S 2
#define   I 3
#define   C 4

// Coherence Requests  (function g()) 
#define LD_MISS 0
#define SD_MISS 1
#define TX_BEGIN 2
#define TX_END 3
#define SD_HIT 4

// Coherence Responses  (function w())
#define NACK 0
#define ACK 1
#define TX_ABORT 2

#define BLKSIZE  5  // So 2^5 bytes per block

#define CACHEBITS 0
#define TVCBITS  CACHEBITS+2
#define CACHESIZE (0x1 << CACHEBITS)  // Size in Blocks
#define TAGSHIFT  (0x1 << (BLKSIZE + CACHEBITS))
#define TVC_TAGSHIFT  (0x1 << (BLKSIZE + TVCBITS))


// utility functions
/**
 * Decode directory state to string
 */
std::string f(int32_t directorystate) {
    std::string ret;
    switch (directorystate) {
        case 1: ret = "M"; break;
        case 2: ret = "S"; break;
        case 3: ret = "I"; break;
        default: ret = "Undefined Directory State\n";
    }
    return ret;
}
/**
 * Decode TVC state to string
 */
std::string m(int32_t tvcstate) {
    std::string ret;
    switch (tvcstate) {
        case 1: ret = "M"; break;
        case 2: ret = "S"; break;
        case 3: ret = "I"; break;
        case 4: ret = "C"; break;
        default: return("Undefined TVC State\n");
    }
    return ret;
}
/**
 * Decode coherence request type to string
 */
std::string g(int32_t coherencerequest) {
    std::string ret;
    switch (coherencerequest) {
        case 0: ret = "LD_MISS"; break;
        case 1: ret = "SD_MISS"; break;
        case 2: ret = "TX_BEGIN"; break;
        case 3: ret = "TX_END"; break;
        case 4: ret = "SD_HIT"; break;            
        default: ret = "Undefined Coherence Request\n";
    }
    return ret;
}
/**
 * Decode transaction command constant to string
 */
std::string h(int32_t type) {
    std::string ret;
    if (type == LOAD) 
        ret = "LOAD";
    else  if (type == STORE) 
        ret = "STORE";
    else if (type == TX_BEGIN) 
        ret = "TX_BEGIN";
    else if (type == TX_END) 
        ret = "TX_END";
    else ret = "Trace file has unknown request\n";
    return ret;
}
/**
 * Decode command type to string
 */
std::string t(int32_t commandtype) {
    std::string ret;
    switch (commandtype) {
        case 0: ret = "LD_MISS"; break;
        case 1: ret = "SD_MISS"; break;
        case 4: ret = "SD_HIT"; break;
        default: ret = "Invalid Command from Directory\n";
    }
    return ret;
}
/**
 * Decode response type to string
 */
std::string w(int32_t responsetype) {
    std::string ret;
    switch (responsetype) {
        case 0: ret = "NACK"; break;
        case 1: ret = "ACK"; break;
        case 2: ret = "TX_ABORT"; break;
        default: ret = "Invalid response\n";
    }
    return ret;
}

// ****************************************************************************************** 
// *********************************** CACHE BLOCK STRUCT *********************************** 
// ****************************************************************************************** 
struct cacheblock {
    int32_t TAG;
    int32_t STATE;
    int32_t T;
    int32_t DATA[0x1 << (BLKSIZE-2)];

    cacheblock() : TAG(0), STATE(0), T(0) {}

    cacheblock(int32_t tag, int32_t state, int32_t t) : TAG(tag), STATE(state), T(t) {}

    // cacheblock(int32_t tag, int32_t state, int32_t t, int32_t data[0x1 << (BLKSIZE-2)]) : TAG(tag), STATE(state), T(t), DATA(data) {} //array initializer must be initializer list
};

// Overload << operator for debug printing
std::ostream& operator<<(std::ostream &o, const cacheblock& cblock) {
    return o << "Cacheblock { TAG: " << cblock.TAG << ", STATE: " << cblock.STATE << ", T: " << cblock.T << " }"; // TODO: add printing for data
}

// ****************************************************************************************** 
// ******************************* GENERIC QUEUE ENTRY STRUCT ******************************* 
// ****************************************************************************************** 

/* Structures used to hold memory requests sent to the memory queue */
struct genericQueueEntry {
    int32_t address; // type conversion for systemC
    int32_t procId;
    int32_t type; // 2 bits to store 4 possible commands
    int32_t data;

    // Default constructor
    genericQueueEntry() : address(0), procId(0), type(0), data(0) {}

    // Parameterized constructor
    genericQueueEntry(int32_t addr, int32_t id, int32_t t, int32_t d) : address(addr), procId(id), type(t), data(d) {}

    bool operator== (const genericQueueEntry& entry) const {
        return (address == entry.address && procId == entry.procId && type == entry.type && data == entry.data);
    }

    // For debugging
    void print() const {
        cout << "MemReq { Addr: " << address << ", procId: " << procId << ", type: " << h(type) << ", data: " << data << " }";
    }
};

// Overload << operator for debug printing
std::ostream& operator<<(std::ostream &o, const genericQueueEntry& entry) {
    return o << "MemReq { Addr: " << entry.address << ", procId: " << entry.procId << ", type: " << h(entry.type) << ", data: " << entry.data << " }";
}

// For waveform tracing? Necessary to pass via initiator/target. do nothing for now...
namespace sc_core {
    void sc_trace(sc_trace_file*, const genericQueueEntry&, const std::string&) {}
}