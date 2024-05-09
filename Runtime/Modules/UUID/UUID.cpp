#include "UUID.h"

#include <stdio.h>

#include "Core/Time/Clock.h"
#include "Core/Random/Random.h"

#ifdef WIN32
#include <Windows.h>
#include <Iphlpapi.h>
#include <Assert.h>

#pragma comment(lib, "iphlpapi.lib")

int UUIDGenerator_getMAC(char* mac_addr) {
    PIP_ADAPTER_INFO AdapterInfo;
    DWORD dwBufLen = sizeof(IP_ADAPTER_INFO);
    //char* mac_addr = (char*)malloc(18);

    AdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
    if (AdapterInfo == NULL) {
        printf("Error allocating memory needed to call GetAdaptersinfo\n");
        //free(mac_addr);
        return -1; // it is safe to call free(NULL)
    }

    // Make an initial call to GetAdaptersInfo to get the necessary size into the dwBufLen variable
    if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(AdapterInfo);
        AdapterInfo = (IP_ADAPTER_INFO*)malloc(dwBufLen);
        if (AdapterInfo == NULL) {
            printf("Error allocating memory needed to call GetAdaptersinfo\n");
            //free(mac_addr);
            return -1;
        }
    }

    if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == NO_ERROR) {
        // Contains pointer to current adapter info
        PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
        do {
            // technically should look at pAdapterInfo->AddressLength
            //   and not assume it is 6.
            //sprintf(mac_addr, "%02X:%02X:%02X:%02X:%02X:%02X",
            //    pAdapterInfo->Address[0], pAdapterInfo->Address[1],
            //    pAdapterInfo->Address[2], pAdapterInfo->Address[3],
            //    pAdapterInfo->Address[4], pAdapterInfo->Address[5]);
            //printf("Address: %s, mac: %s\n", pAdapterInfo->IpAddressList.IpAddress.String, mac_addr);
            // print them all, return the last one.
            // return mac_addr;

            //printf("\n");

            mac_addr[0] = pAdapterInfo->Address[0];
            mac_addr[1] = pAdapterInfo->Address[1];
            mac_addr[2] = pAdapterInfo->Address[2];
            mac_addr[3] = pAdapterInfo->Address[3];
            mac_addr[4] = pAdapterInfo->Address[4];
            mac_addr[5] = pAdapterInfo->Address[5];

            pAdapterInfo = pAdapterInfo->Next;
        } while (pAdapterInfo);
    }
    free(AdapterInfo);

    return 0;
}

#endif // WINDOWS


NAMESPACE_BEGIN

UUIDGenerator::UUIDGenerator()
{
    m_MACAddress = 0;
    if (UUIDGenerator_getMAC((char*)&m_MACAddress))
    {
        assert(0);
    }
}

UUID UUIDGenerator::GetUUID()
{
    auto now = (Clock::ns::now()) / 100;
    //now = now >> 4;
    now = (now << 4) >> 4;

    UUID ret = {};

    ret.part0 = m_MACAddress | (now << 48);

    ret.part1 = (now >> 48) | (int64_t(Random::Rand()) << 12);

	return ret;
}

NAMESPACE_END