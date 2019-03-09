#ifndef KNX_MULTIRINGS_H
#define KNX_MULTIRINGS_H

#include <knx/common.h>

namespace KNX
{

template<typename IdType, typename RingType>
class MultiRings {
public:
    MultiRings(size_t max_rings) : mMaxRings(max_rings){
        mBuffers = new buffers_t[max_rings];
        for(size_t i=0; i<mMaxRings; i++)
            mBuffers[i].free = true;
        mNumFree = mMaxRings;
    }
    
    ~MultiRings() {
        deallocateAll();
        delete[] mBuffers;
    }

    void deallocate(size_t id) {
        if(id >= mMaxRings) return;

        if(!mBuffers[id].free && mBuffers[id].buf->size() == 0) {
            mBuffers[id].free = true;
            mBuffers[id].id = 0;
            delete mBuffers[id].buf;
            mNumFree++;
        }
    }

    void deallocateAll() {
        for(size_t i=0; i<mMaxRings; i++) {
            if(!mBuffers[i].free) {
                mBuffers[i].free = true;
                delete mBuffers[i].buf;
            }
        }
    }

    RingType *get(IdType id) {
        ssize_t firstFree = -1;
        ssize_t rightBuf = -1;
        for(size_t i = 0; i < mMaxRings; i++) {
            if(mBuffers[i].free && firstFree == -1)
                firstFree = i; 
            if(!mBuffers[i].free && mBuffers[i].id == id) {
                rightBuf = i;
                break;
            }
        }

        if(rightBuf != -1) 
            return mBuffers[rightBuf].buf;

        if(firstFree == -1) {
            garbage_collect();
            if(mNumFree == 0)
                return NULL;

            return get(id);
        }

        mBuffers[firstFree].buf = new RingType(1024);
        mBuffers[firstFree].free = false;
        mBuffers[firstFree].id = id;
        mNumFree--;

        return mBuffers[firstFree].buf;
    }

private:
    size_t mNumFree;
    const size_t mMaxRings;

    struct buffers_t {
        bool free;
        IdType id;
        RingType *buf;
    };
    
    buffers_t *mBuffers;

    /* Remove unused rings.. 
     * I know.. the whole class is using a wrong 
     * data structure and should be improved.
     * This is just a test, sorry.
     */
    void garbage_collect() {
        LOGP("Garbage collector. Running out of memory (%lu slots available).",
                mNumFree, mMaxRings);

        if(mNumFree == mMaxRings) 
            return;

        for(size_t i = 0; i < mMaxRings; i++)
            if(!mBuffers[i].free)
                deallocate(i);
    }

    TAG_DEF("MultiRings")
};

};

#endif /* ifndef KNX_MULTIRINGS_H */
