#include "../inc/RECAM_CAM.hpp"

// =====================================
//            RECAM_addressCAM
// =====================================

RECAM_addressCAM::RECAM_addressCAM(int r_spare, int c_spare, int buff_num) : Rs(r_spare), Cs(c_spare)
{
    matrixSize = Rs + Cs;
}

void RECAM_addressCAM::addAddressCAMEntry(Fault &f)
{
    AddressCAMEntry entry = {true, &f, 1, 1, false, false};
    addressCAMEntries.push_back(entry);
}

void RECAM_addressCAM::addAddressCAMEntryFromList(FaultList *fPtr)
{
    // cout << " =========== Loading faults to address CAMs  ===========" << endl;
    for (auto &f : fPtr->pivotFaults)
    {
        addAddressCAMEntry(*f);
    }
}

std::tuple<bool, bool, int> RECAM_addressCAM::updateRowColMust(HybridCAMEntry &hybridCAMEntry)
{
    // cout << "RECAM_addressCAM::updateRowColMust" << endl;
    // cout << "  -Adding non-pivot fault to Hybrid CAM - Row: " << hybridCAMEntry.faultPtr->r << " Col: " << hybridCAMEntry.faultPtr->c << endl;
    // cout << "  -Related Pivot Fault - Row: " << addressCAMEntries[hybridCAMEntry.pointer].faultPtr->r << " Col: " << addressCAMEntries[hybridCAMEntry.pointer].faultPtr->c << endl;
    bool mustCreated = false;
    bool isRowMust = false;
    int halfAddress = -1;

    if (hybridCAMEntry.descriptorRowIsDiff) // non-pivot fault is in the same col, different row with the related pivot fault
    {
        // cout << "  -descriptorRowIsDiff: True. (It will help to classify the non-pivot fault as row or column later.)" << endl;
        addressCAMEntries[hybridCAMEntry.pointer].faultInColCnt++;
        if (addressCAMEntries[hybridCAMEntry.pointer].faultInColCnt > Rs)
        {
            addressCAMEntries[hybridCAMEntry.pointer].colMust = true;
            mustCreated = true;
            isRowMust = false;
            halfAddress = addressCAMEntries[hybridCAMEntry.pointer].faultPtr->r;
            // cout << "Address CAM Entry " << hybridCAMEntry.pointer << " is classified as MUST COL. faultInColCnt: " << addressCAMEntries[hybridCAMEntry.pointer].faultInColCnt << endl;
        }
    }
    else
    {
        // cout << "  -descriptorRowIsDiff: False. It will help to classify the non-pivot fault as row or column later." << endl;
        addressCAMEntries[hybridCAMEntry.pointer].faultInRowCnt++;
        if (addressCAMEntries[hybridCAMEntry.pointer].faultInRowCnt > Cs)
        {
            addressCAMEntries[hybridCAMEntry.pointer].rowMust = true;
            mustCreated = true;
            isRowMust = true;
            halfAddress = addressCAMEntries[hybridCAMEntry.pointer].faultPtr->c;
            // cout << "  -Address CAM Entry " << hybridCAMEntry.pointer << " is classified as MUST ROW. faultInRowCnt: " << addressCAMEntries[hybridCAMEntry.pointer].faultInRowCnt << endl;
        }
    }
    return make_tuple(mustCreated, isRowMust, halfAddress);
}

void RECAM_addressCAM::printAddressCAMEntries()
{
    cout << "========= RECAM_addressCAM :: printAddressCAMEntries ==========" << endl;
    cout << "Address CAM Entries:" << endl;
    for (const auto &entry : addressCAMEntries)
    {
        if (entry.enable && entry.faultPtr != nullptr)
        {
            cout << "Pivot Fault - Subarray: " << entry.faultPtr->SubarrayID
                 << " Channel: " << entry.faultPtr->ChannelID
                 << " Bank: " << entry.faultPtr->BankID
                 << " Row: " << entry.faultPtr->r
                 << " Col: " << entry.faultPtr->c
                 << " faultInRow Count: " << entry.faultInRowCnt
                 << " faultInCol Count: " << entry.faultInColCnt
                 << " Row Must: " << (entry.rowMust ? "Yes" : "No")
                 << " Col Must: " << (entry.colMust ? "Yes" : "No")
                 << endl;
        }
    }
}




// =====================================
//            RECAM_hybridCAM
// =====================================

RECAM_hybridCAM::RECAM_hybridCAM(int r_spare, int c_spare, int buff_num) : Rs(r_spare), Cs(c_spare)
{
    matrixSize = Rs + Cs;
    // The paper sizing rule assumes positive R and C.  At the sweep boundary
    // where exactly one axis is zero, no Hybrid-CAM descriptor is
    // provisioned.  Compute only after handling that boundary so the legacy
    // signed expression cannot become negative or overflow.
    hybridCAMSize = (Rs == 0 || Cs == 0)
        ? 0
        : Rs * (Cs - 1) + Cs * (Rs - 1);
}

bool RECAM_hybridCAM::addHybridCAMEntry(Fault &f, RECAM_addressCAM *addressCAM)
{
    for (size_t idx = 0; idx < addressCAM->addressCAMEntries.size(); ++idx)
    {
        const auto &addressCAMEntry = addressCAM->addressCAMEntries[idx];
        const bool matchRow = addressCAMEntry.faultPtr->r == f.r;
        const bool matchCol = addressCAMEntry.faultPtr->c == f.c;

        if (addressCAMEntry.enable && matchRow)
        {
            if (addressCAMEntry.rowMust)
            {
                return true;
            }

            HybridCAMEntry hybridentry = {
                true, f.c, static_cast<int>(idx), false, &f};
            const bool mustCreated = std::get<0>(
                addressCAM->updateRowColMust(hybridentry));
            if (mustCreated)
            {
                // Count/must evaluation precedes allocation. Once the row is
                // mandatory, reclaim its earlier Hybrid entries and do not
                // allocate an entry for this triggering fault.
                hybridCAMEntries.erase(
                    std::remove_if(
                        hybridCAMEntries.begin(), hybridCAMEntries.end(),
                        [idx](const HybridCAMEntry &entry)
                        {
                            return entry.enable &&
                                   entry.pointer == static_cast<int>(idx) &&
                                   !entry.descriptorRowIsDiff;
                        }),
                    hybridCAMEntries.end());
                return true;
            }
            if (hybridCAMEntries.size() >= static_cast<size_t>(hybridCAMSize))
            {
                hybridCAM_overflow = true;
                return false;
            }
            hybridCAMEntries.push_back(hybridentry);
            ++writeOperations;
            peakEntries = std::max(peakEntries, hybridCAMEntries.size());
            return true;
        }
        else if (addressCAMEntry.enable && matchCol)
        {
            if (addressCAMEntry.colMust)
            {
                return true;
            }

            HybridCAMEntry hybridentry = {
                true, f.c, static_cast<int>(idx), true, &f};
            const bool mustCreated = std::get<0>(
                addressCAM->updateRowColMust(hybridentry));
            if (mustCreated)
            {
                // Column-symmetric must behavior.
                hybridCAMEntries.erase(
                    std::remove_if(
                        hybridCAMEntries.begin(), hybridCAMEntries.end(),
                        [idx](const HybridCAMEntry &entry)
                        {
                            return entry.enable &&
                                   entry.pointer == static_cast<int>(idx) &&
                                   entry.descriptorRowIsDiff;
                        }),
                    hybridCAMEntries.end());
                return true;
            }
            if (hybridCAMEntries.size() >= static_cast<size_t>(hybridCAMSize))
            {
                hybridCAM_overflow = true;
                return false;
            }
            hybridCAMEntries.push_back(hybridentry);
            ++writeOperations;
            peakEntries = std::max(peakEntries, hybridCAMEntries.size());
            return true;
        }
    }
    return false;
}

std::vector<Fault *> RECAM_hybridCAM::addHybridCAMEntryFromList(
    FaultList *fPtr,
    RECAM_addressCAM *addressCAM)
{
    // cout << " =========== Loading faults to hybrid CAMs  ===========" << endl;
    std::vector<Fault *> overflowFaults;
    for (auto &f : fPtr->nonPivotFaults)
    {
        if (!addHybridCAMEntry(*f, addressCAM))
        {
            overflowFaults.push_back(f);
        }
    }
    return overflowFaults;
}

void RECAM_hybridCAM::printHybridCAMEntries()
{
    cout << "========= RECAM_hybridCAM :: printHybridCAMEntries ==========" << endl;
    cout << "Hybrid CAM Entries:" << endl;
    for (const auto &entry : hybridCAMEntries)
    {
        if (entry.enable && entry.faultPtr != nullptr)
        {
            cout << "Non-Pivot Fault - Subarray: " << entry.faultPtr->SubarrayID
                 << " Channel: " << entry.faultPtr->ChannelID
                 << " Bank: " << entry.faultPtr->BankID
                 << " Row: " << entry.faultPtr->r
                 << " Col: " << entry.faultPtr->c
                 << " Related Pivot Fault Pointer: " << entry.pointer
                 << " halfAddr: " << entry.halfAddr
                 << " descriptorRowIsDiff: " << (entry.descriptorRowIsDiff ? "True" : "False")
                 << endl;
        }
    }
}
