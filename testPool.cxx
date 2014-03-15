
#include <cassert>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <limits.h>

#include "logging.h"

#include "poolthread.h"


using namespace devicedb;
using namespace kj;

// fibonacci
#define TEST_THREADS 4
#define WORK fibWork
#define WORK_RUN 2 //12
//#define FIB_START 2
#define WORK_STEP 1
#define WORK_START (ULONG_LONG_MAX/1000)

// primes
//#define TEST_THREADS 4
//#define WORK primeWork
//#define WORK_RUN 10  // will give you a longer run
//#define WORK_STEP ((ULONG_LONG_MAX-1000)/WORK_RUN)
////#define WORK_RUN 100
////#define WORK_STEP 10000
//#define WORK_START (1)

// note: a 64-bit int can't hold anything past fibonacci(93)

bool isPrime (unsigned long long num)
{
    if (num <=1)
        return false;
    else if (num == 2)
        return true;
    else if (num % 2 == 0)
        return false;
    else
    {
        bool prime = true;
        int divisor = 3;
        double num_d = static_cast<double>(num);
        int upperLimit = static_cast<int>(sqrt(num_d) +1);

        while (divisor <= upperLimit)
        {
            if (num % divisor == 0)
                prime = false;
            divisor +=2;
        }
        return prime;
    }
}

unsigned long long fibonacci(unsigned int n)
{
    unsigned long long a = 0, b = 1;
    for(; n > 0; --n)
    {
        b += a;
        a = b-a;
    }
    return a;
}

// a useless work instance to show how workInstance operates...
struct workInstance {
	int id;
	// notice workInstance has no default cstor (which means we must use the creatorFunc option for Pool)
	workInstance() {};
	workInstance(int threadId) : id(threadId) {	}
	void start() {
		DDB_INFO("Starting work instance: %d\n",id);
	}
	void stop() {
		DDB_INFO("Stopping work instance: %d\n",id);
	}
};

struct workResults {
	int id; // this is just to record the thread that did the work for testing purposes
	unsigned long long result;
	bool success;
	kj::Exception *exception;
//	fibResults& operator=(fibResults&& p)
//	    {
//	        std::swap(result, p.result);
//	        std::swap(success, p.success);
//	        std::swap(exception, p.exception);
//	        return *this;
//	    }
};

struct workParams {
	unsigned long long  p; // just to test a parameter in cstor above
	workParams(unsigned long long P) : p(P) { }
	unsigned long long N;
};

struct fibWork {
//	static workResults *work(workParams &p, Pool<workParams,WORK,workResults>::NO_WORK_INSTANCE) { // would be used if you had no Instance object
	static workResults *work(workParams &p, workInstance &inst) {
		workResults *res = new workResults();
		res->result = fibonacci(p.N);
		res->success = true;
		res->id = inst.id;
		return res;
	}
};

// ineffecient prime number finder for something
// CPU constraining to do.
struct primeWork {
	static workResults *work(workParams &p, workInstance &inst) {
		workResults *res = new workResults();
		unsigned long long Q = p.N;
		for(; Q > 0; --Q) {  // finds the first prime number below Q
			if(isPrime(Q))
				break;
		}
		res->result = Q;
		res->success = true;
		res->id = inst.id;
		return res;
	}
};





int main(int argc, char *argv[]) {

	auto results = heapArrayBuilder<Promise<workResults>>(WORK_RUN);
//	kj::Array<Promise<void> > resultsCompletion = kj::heapArray<Promise<void> >(FIB_RUN);

	TWlib::TW_log::getInstance().dat()->setLogLevel(TW_LOG_ALL);
	DDB_INFO("Test Promise based devicedb::Pool\n");

	auto creator = [](int id) -> workInstance* {
			return new workInstance(id);
		};
	Pool<workParams,WORK,workResults,workInstance> myPool(TEST_THREADS,creator);
//	Pool<workParams,WORK,workResults> myPool(TEST_THREADS);


	Pool<workParams,WORK,workResults,workInstance>::PoolEventPort &events = myPool.getEventPort();
//	Pool<workParams,WORK,workResults>::PoolEventPort &events = myPool.getEventPort();
	EventLoop loop(events);

	myPool.start();

	WaitScope waitscope(loop);


	int num = 0;


	for(unsigned long long x=0;x<WORK_RUN;x++) {
		Own<workParams> p = myPool.newParams(WORK_START + (WORK_STEP*x));
		p->N = WORK_START + (WORK_STEP*x);
		assert(p->N == p->p);
		Promise<workResults> &&promise = myPool.submitWork(std::move(p)).eagerlyEvaluate(nullptr); // eagerly evaluate makes it evaluate sooner than later
		Promise<workResults> &&p2 = promise.then([&num](workResults res) -> workResults {
			num++;
			printf("Output (%d): %llu\n",num, res.result);
			return res;
		}).eagerlyEvaluate(nullptr);
		results.add(std::move(p2));
	}

	Promise<Array<workResults> > all = kj::joinPromises(results.finish());

	all.wait(waitscope);




//	p2.wait(waitscope);

	DDB_DEBUG("Before sleep...\n");

//	myPool.waitOnPool(); // will hang the process - this was meant for a test
//	usleep(3000000);


	DDB_DEBUG("Shutting down...\n");

	myPool.shutdown();

	printf("Test Promise based devicedb::Pool\n");


}
