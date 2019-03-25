#ifndef LIBSMOKE_CLIENT_H
#define LIBSMOKE_CLIENT_H

#include "common.h"
#include <signal.h>
#include <stdio.h>
#include <knx/knx.h>
#include <knx/backend/linux-tcp.h>
#include <knx/crypto/mka.h>
#include <knx/crypto/bd1.h>
#include <knx/crypto/bd2.h>
#include <knx/crypto/gdh2.h>
#include <knx/nodecounter/dummy.h>
#include <knx/pktwrapper.h>
#include <knx/crypto/policies.h>
#include <iostream>
#include <knx/telegram.h>
#include <aes.hpp>

class ClientSmoke {
public:
    ClientSmoke (){}

    bool init() {

    }

    bool receive() {

    }

    bool send() {

    }

private:


};

#endif //LIBSMOKE_CLIENT_H
