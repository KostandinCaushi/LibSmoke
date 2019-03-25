#ifndef SHARED_RING_BUFFER_H
#define SHARED_RING_BUFFER_H

namespace KNX
{

class RingBuffer {
    public:
        RingBuffer(size_t size) : bufsize(size) {
            buf = new uint8_t[bufsize];
            pstart = plen = 0;
        }

        ~RingBuffer() {
            delete[] buf; 
        }

        bool write(uint8_t *data, size_t len) {
            if(plen + len > bufsize) {
                LOG("Cannot write. Running out of memory!\n");
                return false;
            }

            size_t x = (pstart + plen) % bufsize;
            if( x + len < bufsize)
                memcpy(buf + x, data, len);
            else {
                // Two pass write
                size_t delta = bufsize - x;
                memcpy(buf + x, data, delta);
                memcpy(buf, data + delta, len - delta);
            }
            plen += len;
            return true;
        }

        bool read(uint8_t *data, size_t len) {
            if(!peek(data, len) || !remove(len))
                return false;

            return true;
        }

        bool remove(size_t len) {
            if(len > plen)
                return false;

            pstart = (pstart + len) % bufsize;
            plen -= len;
            return true;
        }

        bool peek(uint8_t *data, size_t len) const {
            if(len > plen)
                return false;

            if(pstart + len < bufsize)
                memcpy(data, buf + pstart, len);
            else {
                // Two pass read
                size_t delta = bufsize - pstart;
                memcpy(data, buf + pstart, delta);
                memcpy(data + delta, buf, len - delta);
            }

            return true;
        }

        size_t size() const {
            return plen;
        }
    private:
        uint8_t *buf;
        const size_t bufsize;
        size_t pstart, plen;

    TAG_DEF("RingBuffer");
};

}

#endif /* ifndef SHARED_RING_BUFFER_H */
