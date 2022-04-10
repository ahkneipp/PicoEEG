#ifndef DATA_PROC_H
#define DATA_PROC_H

void setupDataProc(size_t nSamples, unsigned int nChannels);
void processDataThread(void);
void reportPower(unsigned int channel);
void reportTimeDomain(unsigned int channel);

#endif
