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
    vector<BufferCAMEntry *> bufferFaults;

    RECAM_bufferCAM(int buff_num) : buff_num(buff_num) {}

    void addBufferCAMEntry(Fault &f)
    {
        if (bufferFaults.size() < buff_num)
        {
            BufferCAMEntry entry;
            entry.faultPtr = &f;
            bufferFaults.push_back(&entry);
        }
        else
        {
            cout << "Unrepairable :Buffer CAM is full. Cannot add more buffer faults." << endl;
        }
    }

    void addBufferCAMEntryFromList(FaultList *fPtr)
    {
        cout << " =========== Loading faults to buffer CAMs  ===========" << endl;
        for (auto &f : fPtr->bufferFaults)
        {
            addBufferCAMEntry(*f);
        }
    }

    void printBufferCAMEntries() {
        cout << "========= RECAM_bufferCAM :: printBufferCAMEntries ==========" << endl;
        cout << "Buffer CAM Entries:" << endl;
        for (const auto& entry : bufferFaults) {
            if (entry->faultPtr != nullptr) {
                cout << "Buffer Fault - LU: " << entry->faultPtr->LogicUnitID
                     << " Layer: " << entry->faultPtr->LayerID
                     << " Bank: " << entry->faultPtr->BankID
                     << " Row: " << entry->faultPtr->r
                     << " Col: " << entry->faultPtr->c
                     << endl;
            }
        }
    }

};



#endif