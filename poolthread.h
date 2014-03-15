
#include "debugit.h"
#include <assert.h>
// shared_ptr
#include <memory>
// libuv
#include <uv.h>
// KJ::Promises
#include <kj/async.h>
// TWlib::TW_fifo - thread safe buffer
#include <TW/tw_fifo.h>
#include <TW/tw_alloc.h>
#include <TW/tw_khash.h>

#include "logging.h"

namespace devicedb {



using namespace kj;
using namespace TWlib;

struct dummyWorkInstance {
	dummyWorkInstance() {}
	void start() {}
	void stop() {}
};


/**
 * creates a pool of threads...
 * Threads accept workParam(s) as work jobs.
 * Each thread in the pool runs the workClass.work() function when work arrived.
 * Each thread pumps out workResult via a kj::Promise<workResult>
 * Immediately upon submitting work to the Pool as kj::Promise<workResult> is received.
<pre>
typename: workParam {
	// Object or type
	// must have a default, zero parameter constructor. Can not be a pure virtual class.
	workParam() {}
}

typename: workResult {
	// can be a pure virtual class
	bool success;            // must have a property called success. This should
	kj::Exception *exception; // should have an exception propety if there is any chance the work can fail.
	workResult() {}          // default constructor
}

typename: workClass {
    // work should return a workResult data structure, allocated on the heap which will, after returning,
	// be managed by the Pool.
	//
	static workResult *work(workParam &param, workInstance &instance); // threadId is just a number associated with the thread which is running this work.
	// if the Pool does not care about work instance's and none are defined, then use:
	static workResult *work(workParam &param, POOL::NO_WORK_INSTANCE); // threadId is just a number associated with the thread which is running this work.

}

typename: workInstance { // a workThread object is associated with each thread
	workInstance() {}  // a default cstor must be present, but is not necessarily used.
	                   //   if a default cstor should not be used, then use Pool(numthread, creatorFunc) where creatorFunc will create the
	                   //   the workInstance object.
	void start() // called before any work is processed by the thread
	void stop();
}

Pool's cstor optionally takes a creatorFunc...  creatorFunc is a pointer to a function which:

workInstance *creator(int threadId) { // threadId is the same number which will later be passed to workClass::work(workParam, threadId)
	return new workInstance();
}

Note: workClass and WorkInstance can be the same time - this is often the use case.
</pre>
 *
 */
template<typename workParam, typename workClass, typename workResult, typename workInstance = dummyWorkInstance>
class Pool {
public:
	typedef dummyWorkInstance& NO_WORK_INSTANCE;
protected:
//
//	class PoolPromiseFulfiller::PromiseFulfiller<workResult> {
//	  // A callback which can be used to fulfill a promise.  Only the first call to fulfill() or
//	  // reject() matters; subsequent calls are ignored.
//
//	public:
//	  virtual void fulfill(workResult&& value) {
//
//	  }
//	  // Fulfill the promise with the given value.
//
//	  virtual void reject(Exception&& exception) {
//
//	  }
//	  // Reject the promise with an error.
//
//	  virtual bool isWaiting() {
//
//	  };
//	  // Returns true if the promise is still unfulfilled and someone is potentially waiting for it.
//	  // Returns false if fulfill()/reject() has already been called *or* if the promise to be
//	  // fulfilled has been discarded and therefore the result will never be used anyway.
//	}


	class PoolPromiseAdapter {
	protected:
		class DummyFulfiller : public PromiseFulfiller<workResult> {
		public:
			  virtual void fulfill(workResult&& value) {};
			  virtual void reject(Exception&& exception) {};
			  virtual bool isWaiting() { return false; };
		};

		static const DummyFulfiller dummy;

		PoolPromiseAdapter(Pool &pool, Own<workParam>& param) :
			  owner(pool),
			  fulfiller(nullptr),
			  paramTitle(),
			  result(nullptr),
			  id(owner.getNextAdapterId())
		  {
			  DDB_DEBUGLT("In PoolPromiseAdapter<no promise>(id:%d)",id);
			  paramTitle = std::move(param); // Own::[move] - see kj/memory.h
			  owner._submitToPool(this);
		  }

	public:
		//inline
	  PoolPromiseAdapter(PromiseFulfiller<workResult>& fulfiller,
	                            Pool &pool,
	                            Own<workParam>& param
	                            )
	      : owner(pool),
	        fulfiller(&fulfiller),
	        paramTitle(),
	        result(nullptr),
	        id(owner.getNextAdapterId())
	  {
		  DDB_DEBUGLT("In PoolPromiseAdapter(id:%d)",id);
		  paramTitle = std::move(param); // Own::[move] - see kj/memory.h
		  owner._submitToPool(this);
//		prev = loop.pollTail;
//	    *loop.pollTail = this;
//	    loop.pollTail = &next;
	  }

	  /**
	   * creates a PoolPromiseAdapter which does not have a corresponding promise
	   * @param pool
	   * @return
	   */
	  static PoolPromiseAdapter *createNoPromiseAdapter(Pool &pool, Own<workParam>& param) {
		  return new PoolPromiseAdapter(pool, param);
	  }

	  void attachResult(workResult *res) {
		  this->result = res;
	  }

	  void complete() {
		  if(result->success) {
//			  workResult &l = *result;
			  fulfiller->fulfill(std::move(*result));             // fulfills Promise created
		  } else {
			  fulfiller->reject(std::move(*result->exception));
		  }
	  }

	  ~PoolPromiseAdapter() noexcept(false) {
		  // must deal with the fact that the Promise may get destroyed before
		  // the Pool completes the work.
		  DDB_DEBUG("~PoolPromiseAdapter()");

		  owner.adapterLookup.remove(id); // remove self from the owner pool's lookup table. This will prevent any waiting work from being accomplished
		  if(result)
			  delete result;
//	    if (prev != nullptr) {
//	      if (next == nullptr) {
//	        loop.pollTail = prev;
//	      } else {
//	        next->prev = prev;
//	      }
//	      *prev = next;
//	    }
	  }

	  void removeFromList() {
		  DDB_DEBUG("removeFromList()");
		  owner.adapterLookup.remove(id); // remove self from the owner pool's lookup table. This will prevent any waiting work from being accomplished

//	    if (next == nullptr) {
//	      loop.pollTail = prev;
//	    } else {
//	      next->prev = prev;
//	    }
//	    *prev = next;
//	    next = nullptr;
//	    prev = nullptr;
	  }


	  Pool &owner;
	  PromiseFulfiller<workResult> *fulfiller;
	  workResult *result;
	  Own<workParam> paramTitle; // the param ownership title. For more on Own - see kj/memory.h
	  uint32_t id;

//	  UnixEventPort& loop;
//	  int fd;
//	  short eventMask;
//	  PromiseFulfiller<short>& fulfiller;
//	  PollPromiseAdapter* next = nullptr;
//	  PollPromiseAdapter** prev = nullptr;
	};




	typedef workInstance* (*createInstanceCB)(int threadNum);

	createInstanceCB creatorFunc;
	int numThreads;
	bool accepting; uv_mutex_t acceptingLock;
	uv_mutex_t controlLock; uv_cond_t startedCond;  // for knowing when the pool is truly running...
	uv_loop_t uvOwnerLoop;


	struct workInfo {
		uint32_t adapterId;
		bool complete;
		workInfo(uint32_t id) : adapterId(id), complete(false) {}
	};

	tw_safeFIFO<workInfo *, TWlib::Allocator<TWlib::Alloc_Std> > inboundWorkQ;
	tw_safeFIFO<workInfo *, TWlib::Allocator<TWlib::Alloc_Std> > finishedWorkQ;

	void stopAccepting() {
		uv_mutex_lock(&acceptingLock);
		accepting = false;
		uv_mutex_unlock(&acceptingLock);
	}

	void startAccepting() {
		uv_mutex_lock(&acceptingLock);
		accepting = true;
		uv_mutex_unlock(&acceptingLock);
	}

	bool isAccepting() {
		bool ret = false;
		uv_mutex_lock(&acceptingLock);
		ret = accepting;
		uv_mutex_unlock(&acceptingLock);
		return ret;
	}



	struct threadInfo {
		int id;
		uv_thread_t handle;
		uv_async_t asyncFini; // async notifier to inform controller that the thread is done processing.
		workInstance *instance;
		Pool<workParam, workClass, workResult, workInstance> *owner;
		tw_safeFIFO<workInfo *, TWlib::Allocator<TWlib::Alloc_Std> > workQ;
		bool shutdown;
		threadInfo() : id(0), workQ(), owner(nullptr), shutdown(false), instance(nullptr) {}
		~threadInfo() { if(instance) delete instance; }
	};

	tw_safeFIFO<threadInfo, TWlib::Allocator<TWlib::Alloc_Std> > threads;
	tw_safeFIFO<uv_thread_t, TWlib::Allocator<TWlib::Alloc_Std> >::iter threadIter;

	tw_safeFIFO<threadInfo *, TWlib::Allocator<TWlib::Alloc_Std> > idleThreads;


	//implements: typedef void (*uv_async_cb)(uv_async_t* handle, int status);
//	static void async_complete(uv_async_t *handle, int status) { // wtf does status do? not sure.
//
//	}

	// called by thread, when a thread is ready for work...
	void readyForWork(threadInfo *info) {
		idleThreads.add(info);
	}


	static void worker(void *_info) {
		threadInfo *info = (threadInfo *) _info;
		workInfo *nextWork = nullptr;
		PoolPromiseAdapter *adapter = nullptr;

		info->instance->start();
		while(!info->shutdown) {
			nextWork = nullptr;
			adapter = nullptr;
			info->owner->readyForWork(info);
			DDB_DEBUGLT("Worker <%d> waiting on work.", info->id);
			if(info->workQ.removeOrBlock( nextWork )) {
				PoolPromiseAdapter *adapter = nullptr;
				DDB_DEBUGLT("  ..Looking up adapterId: %d", nextWork->adapterId);
				if(info->owner->adapterLookup.find(nextWork->adapterId, adapter )) {
					DDB_DEBUGLT("Worker <%d> running work func.", info->id);
					workResult *r = workClass::work(*adapter->paramTitle.get(), *(info->instance));
					adapter->attachResult(r);
					info->owner->finishedWorkQ.add(nextWork);
				} else {
					DDB_WARN("Dropping work, adapter (id:%d) is removed from map.", nextWork->adapterId);
				}
			}
		}
		info->instance->stop();
		// SHUTDOWN:
		DDB_DEBUGLT("Worker <%d> Shutting down.", info->id);
		while(info->workQ.removeOrBlock( nextWork )) { // clean up remaining queue on shutdown if there are any
			PoolPromiseAdapter *adapter = nullptr;
			if(info->owner->adapterLookup.remove(nextWork->adapterId, adapter )) {
				workResult *r = new workResult();
				r->success = false;
				// TODO need .exception set also
				adapter->attachResult(r);
			}
		}
	}

	uv_thread_t managerThread;
	bool managerCreated;

	static void poolThreadMgr(void *_owner) {
		Pool<workParam, workClass, workResult, workInstance> *owner = (Pool<workParam, workClass, workResult, workInstance> *) _owner;

		workInfo *next = nullptr;
		bool haveWork = false;
		threadInfo *usethread = nullptr;
		DDB_DEBUGLT("Pool: poolThreadMgr starting.");

		uv_cond_signal(&owner->startedCond); // signal we are running...

		while(owner->isAccepting()) { // FIXME wrap accepting with a mutex
			next = nullptr;
			haveWork = false;
			usethread = nullptr;

			DDB_DEBUGLT("Pool: poolThreadMgr waiting on work.");
			haveWork = owner->inboundWorkQ.peekOrBlock(next);
			if(haveWork) {
				DDB_DEBUGLT("Pool: haveWork true.");
				// find a thread...
				bool haveone = false;
				while(!haveone) {
					DDB_DEBUGLT("Pool: poolThreadMgr looking for worker...");
					if(!owner->isAccepting()) break; // if it was an interruption to shutdown the Pool, then deal with this.
					haveone = owner->idleThreads.removeOrBlock(usethread);
					// if no threads found, the work is never removed. Loop again.
				}
				if(usethread) {
				// remove the work
					owner->inboundWorkQ.remove(next);
				// submit to thread
					DDB_DEBUGLT("Pool: assigned work to thread: %d", usethread->id);
					usethread->workQ.add(next);
				}
			}
		} // wait for the next one...
		DDB_DEBUGLT("Pool: poolThreadMgr shutting down.");
	}

	uint32_t nextAdapterId; uv_mutex_t adapterIdLock;
	inline uint32_t getNextAdapterId() {
		uint32_t ret = 0;
		uv_mutex_lock(&adapterIdLock);
		ret = nextAdapterId++;
		uv_mutex_unlock(&adapterIdLock);
		return ret;
	}

	TW_KHash_32<uint32_t, PoolPromiseAdapter *, TWlib::TW_Mutex, TWlib::eqstr_numericP<uint32_t>, TWlib::Allocator<TWlib::Alloc_Std> > adapterLookup;

	void _submitToPool(PoolPromiseAdapter *adapter) {
		adapterLookup.addReplace(adapter->id, adapter);
		DDB_DEBUG_TXT( PoolPromiseAdapter *a2 = *adapterLookup.find(adapter->id); assert(a2 == adapter); )

		workInfo *i = new workInfo(adapter->id);
		inboundWorkQ.add(i); // will wake up control thread...
		DDB_DEBUGLT("Worked added to inboundWorkQ (remaining: %d)", inboundWorkQ.remaining());
	}



public:

	// only one instance of this used per Pool:
	class PoolEventPort : public kj::EventPort {
	protected:
		Pool<workParam, workClass, workResult, workInstance> &owner;
	public:

		PoolEventPort( Pool<workParam, workClass, workResult, workInstance> &_owner ) : kj::EventPort(), owner(_owner) {}

		virtual void wait() final {
			workInfo *work = nullptr;
			bool ret = false;
			while(!ret) {
				DDB_DEBUGLT("PoolEventPort: wait()");
				ret = owner.finishedWorkQ.removeOrBlock( work ); // it's possible for a tw_safeFIFO to unblock without data. this deals with that.
			}

			if(ret) {
				PoolPromiseAdapter *adapter = nullptr;
				DDB_DEBUGLT("PoolEventPort: Looking up Adapter (id:%d)...", work->adapterId);
				if(owner.adapterLookup.remove(work->adapterId, adapter )) {
					DDB_DEBUGLT("PoolEventPort: Found Adapter (id:%d) - completing.", work->adapterId);
					adapter->complete(); // fulfills promise
				}
				delete work;
			}
		}

		virtual void poll() final {
			workInfo *work = nullptr;
			bool ret = false;
			ret = owner.finishedWorkQ.remove( work ); // dont block - but look

			DDB_DEBUGLT("PoolEventPort: poll()");

			if(ret) {
				PoolPromiseAdapter *adapter = nullptr;
				if(owner.adapterLookup.remove(work->adapterId, adapter )) {
					adapter->complete(); // fulfills promise
				}
				delete work;
			}
		}
	};


	PoolEventPort &getEventPort() {
		return eventPort;
	}


//	std::function<workInstance* (int)> &creatorFunc;
//std::function<workInstance* (int)> &cstorfunc
	explicit Pool(int num, createInstanceCB cstorfunc = nullptr) : //, uv_loop_t *ownerloop) :
			numThreads(num),
			threads(),
			idleThreads(),
			inboundWorkQ(),
			finishedWorkQ(),
			threadIter(),
//			uvOwnerLoop(ownerloop),
			eventPort(*this),
			accepting(false),
			nextAdapterId(1),
			managerCreated(false),
			creatorFunc(cstorfunc)
	{
		uv_mutex_init(&acceptingLock);
		uv_mutex_init(&adapterIdLock);
		uv_mutex_init(&controlLock);
		uv_cond_init(&startedCond);
		for(int x=0;x<numThreads;x++) {
			threadInfo *info = threads.addEmpty();
			info->owner = this;
			info->id = threads.remaining();
			if(creatorFunc)
				info->instance = creatorFunc(info->id);
			else
				info->instance = new workInstance();
//			uv_async_init(uvOwnerLoop, &info->asyncFini, Pool<workParam, workClass, workResult>::async_complete);
			uv_thread_create(&info->handle, &worker, info);
			accepting = true;
//			idleThreads.add(info); // add to idleThreads list
			DDB_DEBUGLT("Thread %d added to pool.", info->id);
		}
	}

	~Pool() {
		uv_mutex_destroy(&acceptingLock);
		uv_mutex_destroy(&adapterIdLock);
		threadInfo *info = nullptr;
		// threads must be shutdown first...
	}

//	/**
//	 * Submits a job to the pool. Gets a promise immediately back
//	 */
//	kj::Promise<workResult> submit(std::shared_ptr<workParam> *p) {
//
//	}

	void start() {
		startAccepting();
		if(!managerCreated) {
			managerCreated = true;
			uv_thread_create(&managerThread, poolThreadMgr, this);
		}
		uv_cond_wait(&startedCond,&controlLock); // wait until it's running
	}

	void waitOnPool() {
		uv_thread_join(&managerThread);
	}

	void shutdown() {
		stopAccepting();
		inboundWorkQ.unblockAll();
		finishedWorkQ.unblockAll();
		// TODO - go through each thread, flag shutdown and unblock.

	}

	/**
	 * should be called any time the caller needs to create a new workParam for a new job
	 */
	template <typename... Params>
	Own<workParam> newParams(Params&&... params) {
		return kj::heap<workParam>(kj::fwd<Params>(params)...); // returns a new Own title for a new workParam object (allocated on the heap)
	}

	/**
	 * Submits a job to the Pool.<br>
	 * workParam should be declared on the heap using Pool::newParams(). In calling this, the caller yields all control of workparam to the Pool
	 * and the Pool will destroy the workParam object on completion.
	 * @return kj::Promise which will fulfill when the work is complete.
	 */
	Promise<workResult> submitWork(Own<workParam> &&workparamOwn) {
		 // the constructor of PoolPromiseAdapter is going to submit the work to the Pool, etc.
		return newAdaptedPromise<workResult, PoolPromiseAdapter>(*this, workparamOwn);
	}

	/**
	 * Submits a job to the Pool.<br>
	 * workParam should be declared on the heap using Pool::newParams(). In calling this, the caller yields all control of workparam to the Pool
	 * and the Pool will destroy the workParam object on completion.
	 * @param workparamOwn
	 * @return void
	 */
	void submitWorkNoPromise(Own<workParam> &&workparamOwn) {
		 // the constructor of PoolPromiseAdapter is going to submit the work to the Pool, etc.
		PoolPromiseAdapter::createNoPromiseAdapter(*this, workparamOwn);
	}


	void kill() {

	}

protected:
	PoolEventPort eventPort;

};





} // end namespace
