#include <systemc.h>
#include "sct_common.h"
#include "global.h"

// template<class T>
struct FrontEndCacheController : public sc_module 
{
    sc_in<bool>         clk{"clk"};
    sc_in<bool>         nrst{"nrst"};
    
    // cacheblock (&CACHE) [MAX_NUM_PROCESSORS][CACHESIZE];
    cacheblock CACHE[CACHESIZE]; // Private L1 cache

    int32_t procId;

    sct_target<genericQueueEntry>       sem_memreq1{"sem_memreq1"};
    sct_initiator<genericQueueEntry>    sem_memdone1{"sem_memdone1", 1};
    sct_initiator<int32_t>              FEresponse1{"FEresponse1"};
    sct_initiator<int32_t>              FEresult1{"FEresult1"};

    SC_HAS_PROCESS(FrontEndCacheController);
    
    // explicit FrontEndCacheController(const sc_module_name& name, cacheblock (&cacheArray)[MAX_NUM_PROCESSORS][CACHESIZE], const int32_t id) : sc_module(name), CACHE(cacheArray), procId(id)
    explicit FrontEndCacheController(const sc_module_name& name, const int32_t id) : sc_module(name), procId(id)
    {
        sem_memreq1.clk_nrst(clk, nrst);
        sem_memdone1.clk_nrst(clk, nrst);
        FEresponse1.clk_nrst(clk, nrst);
        FEresult1.clk_nrst(clk, nrst);

        SC_THREAD(threadProc);
        sensitive << sem_memreq1 << sem_memdone1 << FEresponse1 << FEresult1;
        async_reset_signal_is(nrst, 0);
    }
    
    void threadProc() {
        cout << "FrontEndCacheController " << procId << ": Activated at time " << sc_time_stamp() << " " << sc_delta_count() << endl;

        sem_memreq1.reset_get();
        sem_memdone1.reset_put();

        genericQueueEntry copyreq1;
        genericQueueEntry copyreq2;
        int32_t cacheLookupResponse;
        int32_t cacheLookupValue;
        int32_t cacheMissResponse;

        // wait();
        
        while(true) {
            wait();
            sem_memdone1.clear_put();
            FEresponse1.clear_put();
            FEresult1.clear_put();
            // cout << "FECC\t: Got clock tick at " << sc_time_stamp() << " " << sc_delta_count() << endl;
            if (sem_memreq1.request()) {
                genericQueueEntry req = sem_memreq1.get();

                if(TRACE)
                    cout << "FrontEndController: Gets request " << req << " at: " << sc_time_stamp() << " " << sc_delta_count() << endl;

                copyreq1 = req;
                copyreq2 = req;

                cacheLookupResponse = LookupCache(copyreq1); // TODO: implement LookupCache

                switch (cacheLookupResponse) {
                    case TX_ABORT:
                        FEresponse1.put(TX_ABORT);
                        // increment numAborts
                        cout << "TX_ABORT" << endl;
                        break;
                    case ACK:
                        FEresponse1.put(ACK);
                        cout << "ACK" << endl;
                        break;
                    case NACK:
                        // do something
                        //copyreq1 = copyreq2?
                        cout << "NACK" << endl;
                        // wait(1); // TODO: cacheMissResponse = handleCacheMiss(copyreq1)
                        switch (cacheMissResponse) {
                            case ACK:
                                FEresponse1.put(ACK);
                                break;
                            case TX_ABORT:
                                FEresponse1.put(TX_ABORT);
                                // increment numAborts
                                break;
                            default:
                                cout << "ERROR: Unrecognized response by handleCacheMiss at " << sc_time_stamp() << " " << sc_delta_count() << endl;
                                break;
                        }
                        break;
                }

                // wait(10);

                if (TRACE)
                    cout << "FrontEnd: Responding" << endl;
                sem_memdone1.put(req);
                // cout << "FECC\t: Put " << data << " at: " << sc_time_stamp() << " " << sc_delta_count() << endl;
            }
        }
    }

    /**
     * Checks cache for requested word. Returns FALSE if the word is not  valid in the cache 
     * (cache miss). On cache hit: a silent transaction is applied directly the cache; returns 
     * after delay of 1  CLOCK_CYCLE.A hit requiring a bus transaction is handled before 
     * returning.
     */
    int32_t LookupCache(genericQueueEntry& req) {
        int32_t blkNum, myTag;
        int32_t cacheOp, address;
        int32_t wordOffset, data;
        int32_t response;
        bool cacheHit = false;
        
        int32_t ret; // hold the return value since code after return isn't allowed

        // Parse the memory request address
        address = req.address;
        data = req.data;

        blkNum = (address >> BLKSIZE) % CACHESIZE;
        myTag = (address >> BLKSIZE) / CACHESIZE;

        // NOTE: be careful here!, using sc uint 32 instead of int. using sizeof doesn't compile, so value is hardcoded to 24
        // NOTE 2: switching to int32_t, has same size as int and can be divided by shifting right by 2.
        wordOffset = (address % (0x1 << (BLKSIZE))) >> 2; // / 24; //sizeof(int32_t); 
        cacheOp = req.type;

        // Check for cache hit
        if ((CACHE[blkNum].TAG == myTag) && (CACHE[blkNum].STATE != I)) {
            cacheHit = true;
            // cout << "here" << endl;
        }
        // myTag = CACHE[blkNum].TAG;

        if (TRACE)
            cout << "LookupCache: proc " << procId << " Op: " << cacheOp << " Cache Hit: " << cacheHit << " Addr " << address << " Time "  << sc_time_stamp() << " " << sc_delta_count() << endl;

        switch (cacheOp) {
            case TX_BEGIN:
                // set coreInTransaction[procId]
                wait(); // response = makeCoherenceRequest(procId, req);
                if (TRACE)
                    cout << "LookupCache: Done TX_BEGIN proc " << procId << " Time "  << sc_time_stamp() << " " << sc_delta_count() << endl;
                ret = ACK;
                break;
            
            case TX_END:
                wait(); // response = makeCoherenceRequest(procId, req);
                // clear coreInTransaction[procId]
                if (TRACE)
                    cout << "LookupCache: Done TX_END proc " << procId << " Time "  << sc_time_stamp() << " " << sc_delta_count() << endl;
                ret = ACK;
                break;
            
            case LOAD:
                if (!cacheHit) {
                    ret = NACK;
                } else {
                    // increment cache read hits
                    req.data = CACHE[blkNum].DATA[wordOffset];
                    // process delay, don't need
                    cout << "LookupCache: proc " << procId <<" LOAD from addr " << address << " Time " << sc_time_stamp() << " " << sc_delta_count() << endl;
                    FEresult1.put(req.data);
                    ret = ACK;
                }
                break;
            case STORE:
                if (!cacheHit) {
                    ret = NACK;
                } else if (CACHE[blkNum].STATE == M) { // Silent Write
                    // increment cache write hits
                    CACHE[blkNum].DATA[wordOffset] = req.data; // Write word to cache
                    // processdelay
                    cout << "LookupCache: proc " << procId <<" Silent STORE Addr " << address << " Time " << sc_time_stamp() << " " << sc_delta_count() << endl;
                    // displayCacheBlock
                    ret = ACK;
                } else if (CACHE[blkNum].STATE == S) { // Need upgrade
                    // increment upgrade counter
                    req.type = SD_HIT;
                    if (TRACE)
                        cout << "LookupCache: proc " << procId << " Requesting UPGRADE at " << sc_time_stamp() << " " << sc_delta_count() << endl;
                    wait(); // response = makeCoherenceRequest(procId, req);

                    if (response == ACK) {
                        CACHE[blkNum].STATE = M; // Upgrade cache block to M
                        CACHE[blkNum].DATA[wordOffset] = req.data; // Write word to cache
                        // processdelay
                        cout << "LookupCache: proc " << procId << " Addr: " << address << " UPGRADED at " << sc_time_stamp() << " " << sc_delta_count() << endl;
                        //displaycacheblock
                        ret = ACK;
                    } else if (response == TX_ABORT) {
                        if (TRACE)
                            cout << "LookupCache: proc " << procId << " got TX_ABORT at " << sc_time_stamp() << " " << sc_delta_count() << endl;
                        cout << "LookupCache: proc " << procId << " will retry SD_HIT as a SD_MISS after delay. Current time: "  << sc_time_stamp() << " " << sc_delta_count() << endl;
                        ret = TX_ABORT;
                    }
                }
                break;
        }
        

        return ret;
    }


    int32_t makeCoherenceRequest(genericQueueEntry& req) {
        int32_t blkNum, blkTag, op;

        genericQueueEntry coherenceReq;

        //Parse Request
        blkNum = (req.address >> BLKSIZE) % CACHESIZE;
        blkTag = (req.address >> BLKSIZE) / CACHESIZE; // NOTE: maybe better to change this to bitshift
        op = req.type;

        cout << "makeCoherenceRequest: Received " << req << " at "  << sc_time_stamp() << " " << sc_delta_count() << endl;

        if ((op == TX_BEGIN) || (op == TX_END)) {
            coherenceReq = req;
            // c
        }
        return 0;

    }
};

 