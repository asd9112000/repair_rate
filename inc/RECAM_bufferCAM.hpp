#ifndef RECAM_BUFFERCAM_HPP
#define RECAM_BUFFERCAM_HPP

#include "./Fault.hpp"

class BufferCAMEntry
{
public:
    Fault *faultPtr = nullptr;
};

class RECAM_bufferCAM
{
public:
    // Add member variables and methods as needed
    int buff_num;
    vector<BufferCAMEntry> bufferFaults;

    RECAM_bufferCAM(int buff_num) : buff_num(buff_num) {}

    void addBufferCAMEntry(Fault &f);
    void addBufferCAMEntryFromList(FaultList *fPtr);
    void printBufferCAMEntries() ;

};



#endif
