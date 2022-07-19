#include <iostream>
#include <thread>
#include <vector>
 
#include "atomic_multilog.h"
#include "confluo_store.h"
#include "rpc_client.h"
#include "rpc_record_batch_builder.h"
#include "schema/schema.h"

// The Standalone C++ client -- multithreaded

void thread_function(size_t n_entries, confluo::atomic_multilog* mlog) {
    confluo::thread_manager::register_thread();
    size_t done = 0;

    std::string s64(61, 'I');
    std::string s128(125, 'I');

    size_t batch_size = 500000;

    while (1) {
        auto batch_bldr = mlog->get_batch_builder();

        for (size_t i=0; i<batch_size && done < n_entries; i++) {
            if (i%2 == 0) {
                batch_bldr.add_record({ "100.0", "0.5", "0.9", s64});
            } else {
                batch_bldr.add_record({ "100.0", "0.5", "0.9", s128});
            }
            done++;
        }

        printf("%lu %f\n", done, done / float(n_entries)*100);
        confluo::record_batch batch = batch_bldr.get_batch();
        mlog->append_batch(batch);

        if (done == n_entries) {
            break;
        }
    }

    printf("Done!\n");
    n_entries = mlog->num_records();
    printf("Inserted %lu entries!\n", n_entries);
    confluo::thread_manager::deregister_thread();
}


int main(int argc, char *argv[]) {
    
    // Parsing the arguments
    if (argc != 2) {
        printf("Usage: ./client.cpp <n_entries>\n");
        return -1;
    }
    size_t n_entries = atol(argv[1]);
    assert(n_entries >= 1);

    printf("Starting the Confluo Store and setting up the Multilog...\n");
    system("rm -rf perf_log");
    
    confluo::confluo_store store("perf_log");
    
    std::string schema_str("{timestamp: ULONG, op_latency_ms: DOUBLE, cpu_util: DOUBLE, mem_avail: DOUBLE, log_msg: STRING(128)}");
    auto schema_vec = parse_schema(schema_str);
    confluo::schema_t schema(schema_vec);
    auto storage_mode = confluo::storage::IN_MEMORY;
    
    store.create_atomic_multilog("perf_log", schema_vec, storage_mode);
    confluo::atomic_multilog* mlog = store.get_atomic_multilog("perf_log");
    
    mlog->add_index("op_latency_ms");
    mlog->add_filter("low_resources", "cpu_util>0.8 || mem_avail<0.1");
    mlog->add_aggregate("max_latency_ms", "low_resources", "MAX(op_latency_ms)");
    mlog->install_trigger("high_latency_trigger", "max_latency_ms > 1000");
    
    printf("Inserting %lu entries...\n", n_entries);
    std::vector<std::thread> threads;

    for(int i = 0; i < 2; ++i){
        threads.push_back(std::thread(thread_function, n_entries, mlog));
    }

    for(auto& thread : threads){
        thread.join();
    }
    
    
    std::cout<<"Exit of Main function"<<std::endl;
    

    return 0;
} 

