#include <mutex>

class Lock{

    mutex *mylock;
public:
    explicit Lock(mutex *tem){
        tem->lock();
        mylock  = tem;
    }

    ~Lock(){
        mylock->unlock();
    }
};

mutex cache_mutex;
mutex log_mutex;