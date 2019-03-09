#ifndef KNX_TIMING_H
#define KNX_TIMING_H

#include <ctime>
#include <vector>
#include <queue>

namespace KNX {

namespace Timing {

/* Wanna add benchmarks? Append the name in this list */
#define BENCHMARK_LIST(X) \
X(bd1_compute_key)        \
X(bd1_gen_point)          \
X(bd1_gen_value)          \
X(bd1_init)               \
X(bd2_calc_key)           \
X(bd2_export_key)         \
X(bd2_import_val)         \
X(bd2_init)               \
X(bd2_make_key)           \
X(bd2_make_public)        \
X(bd2_make_val)           \
X(bd2_read_public)        \
X(gdh2_compute_key)       \
X(gdh2_export_key)        \
X(gdh2_init)              \
X(gdh2_make_firstval)     \
X(gdh2_make_val)          \
X(mka_acc_broadval)       \
X(mka_compute_key)        \
X(mka_init)               \
X(mka_make_broadval)      \
X(mka_set_part)           \
X(pktwrapper_packetize)   \
X(pktwrapper_recompose)   \
X(tcp_waiting)            \
X(tcp_read)               \
X(uart_writing)           \
X(uart_waiting)           \
X(uart_read)              \
X(counter_init)           \
X(counter_counting)       \
X(keyexchange_running)    \
X(total_exchange_time)                     

/* --- Do not touch under this line if you care about your mental health --- */

#ifdef ENABLE_BENCHMARKS
/* Keep in sync both enum and string array */
#define __BMARK_ENUM(x) EE_##x,
#define __BMARK_STR(x) #x,

enum benchmark_enum { 
    BENCHMARK_LIST(__BMARK_ENUM) 
    BMARK_SIZE  /* Use BMARK_SIZE as number-of-benchmarks */
};
static const char *benchmark_str[] = { 
    BENCHMARK_LIST(__BMARK_STR)
};

#endif /* ENABLE_BENCHMARKS */

#undef __BMARK_ENUM
#undef __BMARK_STR
#undef BENCHMARK_LIST

#ifdef ENABLE_BENCHMARKS
#define BENCHMARK_INIT() KNX::Timing::Benchmark::init()
#define BENCHMARK_START(id) KNX::Timing::Benchmark::start(KNX::Timing::EE_##id)
#define BENCHMARK_STOP(id) KNX::Timing::Benchmark::store(KNX::Timing::EE_##id)
#define BENCHMARK_PRINT() KNX::Timing::Benchmark::print()
#else
#define BENCHMARK_INIT()
#define BENCHMARK_START(id) 
#define BENCHMARK_STOP(id)
#define BENCHMARK_PRINT()
#endif

#ifdef BENCHMARK_KEYEXCHANGE
#define KEYBENCHMARK_START(id) BENCHMARK_START(id)
#define KEYBENCHMARK_STOP(id) BENCHMARK_STOP(id)
#else
#define KEYBENCHMARK_START(id)
#define KEYBENCHMARK_STOP(id) 
#endif

#ifdef BENCHMARK_NODECOUNTER
#define CTRBENCHMARK_START(id) BENCHMARK_START(id)
#define CTRBENCHMARK_STOP(id) BENCHMARK_STOP(id)
#else
#define CTRBENCHMARK_START(id)
#define CTRBENCHMARK_STOP(id) 
#endif

#ifdef BENCHMARK_NETWORKING
#define NETBENCHMARK_START(id) BENCHMARK_START(id)
#define NETBENCHMARK_STOP(id) BENCHMARK_STOP(id)
#else
#define NETBENCHMARK_START(id)
#define NETBENCHMARK_STOP(id) 
#endif

#if defined(BENCHMARK_KEYEXCHANGE) && defined(BENCHMARK_NODECOUNTER) && \
    defined(BENCHMARK_NETWORKING)
#define ALLBENCHMARK_START(id) BENCHMARK_START(id)
#define ALLBENCHMARK_STOP(id) BENCHMARK_STOP(id)
#else
#define ALLBENCHMARK_START(id)
#define ALLBENCHMARK_STOP(id) 
#endif

#ifdef ENABLE_BENCHMARKS

typedef struct {
    size_t id;
    uint64_t start;
    uint64_t end;
} timetable_t;

typedef struct delta_t {
    size_t id;
    uint64_t delta;
    bool operator<(const delta_t &rhs) const { return delta > rhs.delta; }
} delta_t;

#if defined(_MIOSIX)
#define TIME_UNIT "ms"
#define TIME_DIVIDER 1
#elif defined(__unix)
#define TIME_UNIT "us"
#define TIME_DIVIDER 1000
#endif

class Benchmark {
public:
    static __attribute__((always_inline)) void start(size_t id) {
        starttimes[id] = gettime();
    }

    static __attribute__((always_inline)) void store(size_t id) {
        timetable_t t;
        timing_t end = gettime();
        t.id = id;
        t.start = diff(table_reference, starttimes[id]);
        t.end = diff(table_reference, end);
        table.push_back(t); 
    }

    static void print() {
        uint64_t sum[BMARK_SIZE] = {0};
        uint64_t starttime = 0;
        uint64_t endtime = 0;
        std::priority_queue<delta_t> data;

        for(size_t i = 0; i < table.size(); i++) {
            timetable_t &t = table[i];
            sum[t.id] += (table[i].end - table[i].start);
            if(table[i].end > endtime)
                endtime = table[i].end;
            if(table[i].start < starttime || starttime == 0)
                starttime = table[i].start;
        }

        for(size_t i=0; i < BMARK_SIZE; i++)
            data.push({i,sum[i]}); 

        /* Draw chart */
        static const int len = 80;
        char line[len+1] = {0};
        char num[30] = {0};

        printf("____Timing report____");
        printf("0");
        for(size_t i = 0; i < len - 23; i++)
            printf("_");
        sprintf(num, "%020lu", (local_lu_t)
                    ((endtime - starttime) / TIME_DIVIDER));
        for(size_t i = 0; num[i] == '0'; i++)
            num[i] = '_';

        printf("%s " TIME_UNIT "+\n", num);

        for(size_t i = 0; i < BMARK_SIZE; i++ ) {
            memset(line, ' ', len); 
            bool not_zero = false;

            /** I know, O(n^2) wrt number of benchmarks */
            for(size_t j = 0; j < table.size(); j++) {
                timetable_t &t = table[j];
                /** Add stars where benchmark is done */
                if(t.id == i) {
                    uint32_t s = (t.start - starttime) * len 
                               / (endtime - starttime);
                    uint32_t e = (t.end - starttime) * len 
                               / (endtime - starttime);

                    /* Use . if used a little */
                    if(s == e && line[s] == ' ') 
                        line[s] = '.';
                    else
                        for(size_t k=s; k<e; k++)
                            line[k] = '*'; 
                    not_zero = true;
                }
            }

            if(not_zero)
                printf("%20s |%s|\n", benchmark_str[i], line);
        }

        for(size_t i = 0; i < len + 23; i++)
            printf("-");
        printf("\n");
        /* Print stats */
        while(!data.empty()) {
            const delta_t &t = data.top();
            if(t.delta/TIME_DIVIDER > 0)
                printf("%17lu " TIME_UNIT " : %s\n", 
                        (local_lu_t)(t.delta/TIME_DIVIDER), 
                        benchmark_str[t.id]);
            data.pop();
        }

    }

    static void init() {
        table_reference = gettime(); 
    }
#if defined(_MIOSIX)
    typedef unsigned long long timing_t;
    typedef uint32_t local_lu_t;
private:
    static uint64_t diff(const timing_t& start, const timing_t& end) {
        return end - start;
    }

    static __attribute__((always_inline)) timing_t gettime() {
        const uint64_t tickToMs = 1000 / miosix::TICK_FREQ;
        return tickToMs * (uint64_t) miosix::getTick();
    }
#elif defined(__unix)
    typedef struct timespec timing_t;
    typedef uint64_t local_lu_t;
private:
    static uint64_t diff(const timing_t& start, const timing_t& end) {
        struct timespec t;

        if((end.tv_sec < start.tv_sec) || 
           (end.tv_sec == start.tv_sec && (end.tv_nsec < start.tv_nsec))) {
            LOG("diff has start > end!");
            return 0;
        }

        if(end.tv_nsec >= start.tv_nsec) {
            t.tv_sec  = end.tv_sec - start.tv_sec;
            t.tv_nsec = end.tv_nsec - start.tv_nsec;
        } else {
            t.tv_sec  = end.tv_sec - start.tv_sec - 1;
            t.tv_nsec = (1000000000 + end.tv_nsec) - start.tv_nsec;
        }

        return t.tv_sec * 1000000000 + t.tv_nsec;
    }

    static __attribute__((always_inline)) timing_t gettime() {
        struct timespec t;

        if(clock_gettime(CLOCK_MONOTONIC, &t) == -1) {
            LOG("clock_gettime failed.\n");
            exit(-1);
        }

        return t;
    }
#else
#error "Timing.h must be implemented for your operating system"
#endif /* Operating system */

    static timing_t starttimes[KNX::Timing::BMARK_SIZE];
    static timing_t table_reference;
    static std::vector<timetable_t> table;

    TAG_DEF("Timing")
};

Benchmark::timing_t Benchmark::starttimes[KNX::Timing::BMARK_SIZE];
Benchmark::timing_t Benchmark::table_reference;
std::vector<timetable_t> Benchmark::table;

#endif /* ENABLE_BENCHMARKS */

} /* Timing */

} /* KNX */

#endif /* ifndef KNX_TIMING_H */
