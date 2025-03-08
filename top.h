#include "sct_assert.h"
#include <systemc.h>
#include "mem.h"
#include "global.h"




class simple_test : public sc_module 
{
public:
    using T = genericQueueEntry;

    sc_in<bool>         clk{"clk"};
    sc_in<bool>         nrst{"nrst"};

    sct_initiator<T>    sem_memreq1{"sem_memreq1"};
    sct_target<T>       sem_memdone1{"sem_memdone1"};
    sct_target<int32_t> FEresponse1{"FEresponse1"};
    sct_target<int32_t> FEresult1{"FEresult1"};

    // struct cacheblock CACHE[MAX_NUM_PROCESSORS][CACHESIZE]; // CACHE "global" variable is passed by reference to every module instance that needs it
    // CACHE[0][0] = cacheblock(1, 2, 3);

    // FrontEndCacheController        a{"a", CACHE, 1};
    FrontEndCacheController         fecc1{"fecc1", 1};
    // FrontEndCacheController          a{"a", 1};

    SC_HAS_PROCESS(simple_test);

    explicit simple_test(const sc_module_name& name) : sc_module(name)
    {
        sem_memreq1.clk_nrst(clk, nrst);
        sem_memdone1.clk_nrst(clk, nrst);
        FEresponse1.clk_nrst(clk, nrst);
        FEresult1.clk_nrst(clk, nrst);
        fecc1.clk(clk);
        fecc1.nrst(nrst);
        fecc1.sem_memreq1.bind(sem_memreq1);
        fecc1.sem_memdone1.bind(sem_memdone1);
        fecc1.FEresponse1.bind(FEresponse1);
        fecc1.FEresult1.bind(FEresult1);
        
        SC_THREAD(init_thread);
        sem_memreq1.addTo(sensitive);
        sem_memdone1.addTo(sensitive);
        FEresponse1.addTo(sensitive);
        FEresult1.addTo(sensitive);
        async_reset_signal_is(nrst, 0);
    }
    
    const unsigned N = 3;
    void init_thread()
    {
        // cacheblock temp(1, 2, 3);
        // CACHE[0][0] = temp;
        cout << "Initiator thread started at " << SCT_CMN_TLM_MODE << sc_time_stamp() << " " << sc_delta_count()  << endl;

        T data(1, 1, 3, 42); 
        T data2;
        sem_memreq1.reset_put();
        sem_memdone1.reset_get();
        int32_t resp;
        wait();

        // data = genericQueueEntry(1, 1, 3, 42);
        while (!sem_memreq1.put(data)) wait();
        cout << "Top\t: Put " << genericQueueEntry(1, 1, 3, 42) << " at: " << sc_time_stamp() << " " << sc_delta_count() << endl;
        // wait();
        
        while (!sem_memdone1.get(data)) wait();
        cout << "Top\t: Got " << data << " at: " << sc_time_stamp() << " " << sc_delta_count() << endl;
        FEresponse1.get(resp);
        cout << "TOP< GOT " << resp << endl;

        
        cout << sc_time_stamp() << " " << sc_delta_count() << " : #2 all get done " << endl;
        wait();

        while (true) {
            wait();
        }
    }
    // genericQueueEntry buildTransaction (int32_t address, int32_t procId, sc_uint<2> type, int32_t data) {
    //     return genericQueueEntry(address, procId, type, data);
    // }
};


