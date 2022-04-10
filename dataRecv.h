#ifndef DATA_RECV_H
#define DATA_RECV_H

void initDataRecvr(int nSamples, int nChannels, int sampleRate);
void setupADC(int nChannels, int samplePeriodMs);
void recvThreadMain();

#endif
