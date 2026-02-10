#include "../inc/RECAM_CAM.hpp"

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
    cout << " =========== Loading faults to address CAMs  ===========" << endl;
    for (auto &f : fPtr->pivotFaults)
    {
        addAddressCAMEntry(*f);
    }
}

std::tuple<bool, bool, int> RECAM_addressCAM::updateRowColMust(HybridCAMEntry &hybridCAMEntry)
{
    cout << "RECAM_addressCAM::updateRowColMust" << endl;
    cout << "  -Adding non-pivot fault to Hybrid CAM - Row: " << hybridCAMEntry.faultPtr->r << " Col: " << hybridCAMEntry.faultPtr->c << endl;
    cout << "  -Related Pivot Fault - Row: " << addressCAMEntries[hybridCAMEntry.pointer].faultPtr->r << " Col: " << addressCAMEntries[hybridCAMEntry.pointer].faultPtr->c << endl;
    bool mustCreated = false;
    bool isRowMust = false;
    int halfAddress = -1;

    if (hybridCAMEntry.descriptorRowIsDiff) // non-pivot fault is in the same col, different row with the related pivot fault
    {
        cout << "  -descriptorRowIsDiff: True. (It will help to classify the non-pivot fault as row or column later.)" << endl;
        addressCAMEntries[hybridCAMEntry.pointer].faultInColCnt++;
        if (addressCAMEntries[hybridCAMEntry.pointer].faultInColCnt > Rs)
        {
            addressCAMEntries[hybridCAMEntry.pointer].colMust = true;
            mustCreated = true;
            isRowMust = false;
            halfAddress = addressCAMEntries[hybridCAMEntry.pointer].faultPtr->r;
            cout << "Address CAM Entry " << hybridCAMEntry.pointer << " is classified as MUST COL. faultInColCnt: " << addressCAMEntries[hybridCAMEntry.pointer].faultInColCnt << endl;
        }
    }
    else
    {
        cout << "  -descriptorRowIsDiff: False. It will help to classify the non-pivot fault as row or column later." << endl;
        addressCAMEntries[hybridCAMEntry.pointer].faultInRowCnt++;
        if (addressCAMEntries[hybridCAMEntry.pointer].faultInRowCnt > Cs)
        {
            addressCAMEntries[hybridCAMEntry.pointer].rowMust = true;
            mustCreated = true;
            isRowMust = true;
            halfAddress = addressCAMEntries[hybridCAMEntry.pointer].faultPtr->c;
            cout << "  -Address CAM Entry " << hybridCAMEntry.pointer << " is classified as MUST ROW. faultInRowCnt: " << addressCAMEntries[hybridCAMEntry.pointer].faultInRowCnt << endl;
        }
    }
    return make_tuple(mustCreated, isRowMust, halfAddress);
}





RECAM_hybridCAM::RECAM_hybridCAM(int r_spare, int c_spare, int buff_num) : Rs(r_spare), Cs(c_spare)
{
    matrixSize = Rs + Cs;
    hybridCAMSize = Rs * (Cs - 1) + Cs * (Rs - 1); // the maximum number of non-pivot faults that can be stored in hybrid CAM, which is when all the non-pivot faults are related to different pivot faults
}

void RECAM_hybridCAM::addHybridCAMEntry(Fault &f, RECAM_addressCAM *addressCAM)
{
    cout << "RECAM_hybridCAM::addHybridCAMEntry" << endl;
    cout << "  -Trying to add non-pivot fault to Hybrid CAM - Row: " << f.r << " Col: " << f.c << endl;

    bool findRelatedPivotFault = false;
    for (int idx = 0; idx < addressCAM->addressCAMEntries.size(); ++idx)
    {
        const auto &addressCAMEntry = addressCAM->addressCAMEntries[idx];
        bool matchRow = addressCAMEntry.faultPtr->r == f.r;
        bool matchCol = addressCAMEntry.faultPtr->c == f.c;
        // auto [mustCreated, isRowMust, halfAddress] = make_tuple(false, false, -1);

        if (addressCAMEntry.enable && matchRow)
        {
            if (addressCAMEntry.rowMust)
            {
                findRelatedPivotFault = true;
                cout << "  -Skipped Fault Row: " << f.r << " Col: " << f.c << " (It's already classified as must row.)" << endl;
                break;
            }

            HybridCAMEntry hybridentry = {true, f.c, idx, false, &f};
            hybridCAMEntries.push_back(hybridentry);
            cout << "  -Adding Hybrid CAM Entry: Row: " << f.r << " Col: " << f.c << " pointer: " << idx << " descriptorRowIsDiff: " << hybridentry.descriptorRowIsDiff << endl;

            auto [mustCreated, isRowMust, halfAddress] = addressCAM->updateRowColMust(hybridentry);
            if (mustCreated)
            {
                // if (isRowMust)
                // {
                for (int k = hybridCAMEntries.size() - 1; k >= 0; k--)
                {
                    // if (hybridCAMEntries[k].enable && hybridCAMEntries[k].halfAddr == halfAddress && hybridCAMEntries[k].descriptor_row)
                    if (hybridCAMEntries[k].enable && hybridCAMEntries[k].pointer == idx && !hybridCAMEntries[k].descriptorRowIsDiff)
                    {
                        cout << "  -Removing Hybrid CAM Entry: Row: " << hybridCAMEntries[k].faultPtr->r << " Col: " << hybridCAMEntries[k].faultPtr->c << endl;
                        cout << "  -Removing Hybrid CAM Entry with pointer: " << hybridCAMEntries[k].pointer << " descriptorRowIsDiff: " << (hybridCAMEntries[k].descriptorRowIsDiff ? "True" : "False") << endl;
                        hybridCAMEntries.erase(hybridCAMEntries.begin() + k);
                    }
                }
                // }
            }
            findRelatedPivotFault = true;
        }
        else if (addressCAMEntry.enable && matchCol)
        {
            if (addressCAMEntry.colMust)
            {
                findRelatedPivotFault = true;
                cout << "  -Skipped Fault Row: " << f.r << " Col: " << f.c << " (It's already classified as must col.)" << endl;
                break;
            }

            HybridCAMEntry hybridentry = {true, f.c, idx, true, &f};
            hybridCAMEntries.push_back(hybridentry);
            cout << "  -Adding Hybrid CAM Entry: Row: " << f.r << " Col: " << f.c << " pointer: " << idx << " descriptorRowIsDiff: " << hybridentry.descriptorRowIsDiff << endl;

            auto [mustCreated, isRowMust, halfAddress] = addressCAM->updateRowColMust(hybridentry);
            if (mustCreated)
            {
                // if (!isRowMust)
                // {
                    for (int k = hybridCAMEntries.size() - 1; k >= 0; k--)
                    {
                        if (hybridCAMEntries[k].enable && hybridCAMEntries[k].pointer == idx && hybridCAMEntries[k].descriptorRowIsDiff)
                        {
                            cout << "  -Removing Hybrid CAM Entry: Row: " << hybridCAMEntries[k].faultPtr->r << " Col: " << hybridCAMEntries[k].faultPtr->c << endl;
                            cout << "  -Removing Hybrid CAM Entry with pointer: " << hybridCAMEntries[k].pointer << " descriptorRowIsDiff: " << (hybridCAMEntries[k].descriptorRowIsDiff ? "True" : "False") << endl;
                            hybridCAMEntries.erase(hybridCAMEntries.begin() + k);
                        }
                    }
                // }
            }
            findRelatedPivotFault = true;
        }

        if (hybridCAMEntries.size() > hybridCAMSize)
        {
            hybridCAM_overflow = true;
            cout << "Error :Hybrid CAM overflow! Current size: " << hybridCAMEntries.size() << ", Max size: " << hybridCAMSize << endl;
        }

        if (findRelatedPivotFault)
        {
            break;
        }
    }
}

void RECAM_hybridCAM::addHybridCAMEntryFromList(FaultList *fPtr, RECAM_addressCAM *addressCAM)
{
    cout << " =========== Loading faults to hybrid CAMs  ===========" << endl;
    for (auto &f : fPtr->nonPivotFaults)
    {
        addHybridCAMEntry(*f, addressCAM);
    }
}
