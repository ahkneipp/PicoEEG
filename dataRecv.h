#ifndef DATA_RECV_H
#define DATA_RECV_H

void initDataRecvr(unsigned int nSamples, 
		unsigned int nChannels, unsigned int samplePeriodus);
void setupADC(unsigned int nChannels, unsigned int samplePeriodus);
void recvThreadMain();

#endif
