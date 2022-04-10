#ifndef DATA_RECV_H
#define DATA_RECV_H

void initDataRecvr(int nSamples, int nChannels, int samplePeriodus);
void setupADC(int nChannels, int samplePeriodus);
void recvThreadMain();

#endif
