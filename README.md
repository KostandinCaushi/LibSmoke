# LibSmoke
LibSmoke is a library that allows embedded systems to communicate between each others in an efficient and secure way.

It integrates together 2 libraries :
* [AES](https://github.com/kokke/tiny-AES-c), in particular AES256_CTR, used to encrypt and decrypt pkts data;
* SKNX (Securing KNX), used to exchange pkts between different devices and to generate a shared key.

### Design & Implementation
