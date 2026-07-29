#include "../inc/RECAM_bufferCAM.hpp"




void RECAM_bufferCAM::addBufferCAMEntry(Fault &f)
{
    if (bufferFaults.size() < buff_num)
    {
        BufferCAMEntry entry;
        entry.faultPtr = &f;
        bufferFaults.push_back(entry);
    }
    else
    {
        cout << "Unrepairable :Buffer CAM is full. Cannot add more buffer faults." << endl;
    }
}

void RECAM_bufferCAM::addBufferCAMEntryFromList(FaultList *fPtr)
{
    // cout << " =========== Loading faults to buffer CAMs  ===========" << endl;
    for (auto &f : fPtr->bufferFaults)
    {
        addBufferCAMEntry(*f);
    }
}

void RECAM_bufferCAM::printBufferCAMEntries()
{
    cout << "========= RECAM_bufferCAM :: printBufferCAMEntries ==========" << endl;
    cout << "Buffer CAM Entries:" << endl;
    for (const auto& entry : bufferFaults) {
        if (entry.faultPtr != nullptr) {
            cout << "Buffer Fault - Subarray: " << entry.faultPtr->SubarrayID
                    << " Channel: " << entry.faultPtr->ChannelID
                    << " Bank: " << entry.faultPtr->BankID
                    << " Row: " << entry.faultPtr->r
                    << " Col: " << entry.faultPtr->c
                    << endl;
        }
    }
}
