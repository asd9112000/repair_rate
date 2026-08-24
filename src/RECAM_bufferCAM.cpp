#include "../inc/RECAM_bufferCAM.hpp"




bool RECAM_bufferCAM::addBufferCAMEntry(Fault &f)
{
    if (bufferFaults.size() < buff_num)
    {
        BufferCAMEntry entry;
        entry.faultPtr = &f;
        bufferFaults.push_back(entry);
        return true;
    }
    return false;
}

bool RECAM_bufferCAM::addBufferCAMEntryFromList(FaultList *fPtr)
{
    // cout << " =========== Loading faults to buffer CAMs  ===========" << endl;
    for (auto &f : fPtr->bufferFaults)
    {
        if (!addBufferCAMEntry(*f))
        {
            return false;
        }
    }
    return true;
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
