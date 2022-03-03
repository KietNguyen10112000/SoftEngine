#pragma once

#include <cstdio>

#ifdef _WIN32

#include <Windows.h>

class Random
{
private:
	inline static HCRYPTPROV hCryptProv = 0;
    inline static BYTE randomBytes[16];

private:
    inline static void GetRandomBytes()
    {
        if (!CryptGenRandom(hCryptProv, 8, randomBytes))
        {
            size_t t = GetTickCount64();
            ::memcpy(randomBytes, &t, ARRAYSIZE(randomBytes));
        }
    };

public:
    inline static void Initialize()
    {
        auto UserName = L"Random_SoftEngine";
        if (CryptAcquireContext(
            &hCryptProv,               // handle to the CSP
            UserName,                  // container name 
            NULL,                      // use the default provider
            PROV_RSA_FULL,             // provider type
            0))                        // flag values
        {
            
        }
        else
        {
            //-------------------------------------------------------------------
            // An error occurred in acquiring the context. This could mean
            // that the key container requested does not exist. In this case,
            // the function can be called again to attempt to create a new key 
            // container. Error codes are defined in Winerror.h.
            if (GetLastError() == NTE_BAD_KEYSET)
            {
                if (CryptAcquireContext(
                    &hCryptProv,
                    UserName,
                    NULL,
                    PROV_RSA_FULL,
                    CRYPT_NEWKEYSET))
                {
                    printf("A new key container has been created.\n");
                }
                else
                {
                    printf("Could not create a new key container.\n");
                    exit(1);
                }
            }
            else
            {
                printf("A cryptographic service handle could not be "
                    "acquired.\n");
                exit(1);
            }
        }
    };

    inline static void UnInitialize()
    {
        if (CryptReleaseContext(hCryptProv, 0))
        {
            //printf("The handle has been released.\n");
        }
        else
        {
            //printf("The handle could not be released.\n");
        }
    }

public:
    inline static float Float(float min, float max)
    {
        GetRandomBytes();
        srand(*(uint32_t*)randomBytes);
        float scale = rand() / (float)RAND_MAX; /* [0, 1.0] */
        return min + scale * (max - min);
    };

    inline static int Int32(int min, int max)
    {
        GetRandomBytes();
        srand(*(uint32_t*)randomBytes);
        return min + rand() % (max + 1 - min);
    };

    inline static long long Int64(long long min, long long max)
    {
        GetRandomBytes();
        srand(*(uint32_t*)randomBytes);
        return min + rand() % (max + 1 - min);
    }

};

#else

static_assert(L"source file must be rewrite");

#endif // _WIN32


