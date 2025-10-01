#include "EC.Misc.h"
#include "SyringeEx.h"
#include <random>
#include <Windows.h>

//Original: EXPANDMD%02d.MIX at 0x82668C
void RedirectExpandMD(const char* NewTarget)
{
    //5301C5 -> 82668C
    ForcedMemcpy(reinterpret_cast<void*>(0x5301C5), &NewTarget, sizeof(const char*));
}
//Original: ELOCAL*.MIX at 0x826614
void RedirectELocal(const char* NewTarget)
{
    //5300DC -> 826614
    ForcedMemcpy(reinterpret_cast<void*>(0x5300DC), &NewTarget, sizeof(const char*));
}
//Original: ECACHE*.MIX at 0x826620
void RedirectECache(const char* NewTarget)
{
    //530016 -> 826620
    ForcedMemcpy(reinterpret_cast<void*>(0x530016), &NewTarget, sizeof(const char*));
}
//Original: MOVIES%02d.MIX at 0x81C200
void RedirectMovies(const char* NewTarget)
{
    //47968C -> 81C200
    //5311EC -> 81C200
    ForcedMemcpy(reinterpret_cast<void*>(0x47968C), &NewTarget, sizeof(const char*));
    ForcedMemcpy(reinterpret_cast<void*>(0x5311EC), &NewTarget, sizeof(const char*));
}
//Original: MOVMD%02d.MIX at 0x81C210
void RedirectMovMD(const char* NewTarget)
{
    //4795DF -> 81C210
    //531181 -> 81C210
    ForcedMemcpy(reinterpret_cast<void*>(0x4795DF), &NewTarget, sizeof(const char*));
    ForcedMemcpy(reinterpret_cast<void*>(0x531181), &NewTarget, sizeof(const char*));
}
//Original: MAPS%02d.MIX at 0x81C2C4
void RedirectMaps(const char* NewTarget)
{
    //47942D -> 81C2C4
    //530ACF -> 81C2C4
    ForcedMemcpy(reinterpret_cast<void*>(0x47942D), &NewTarget, sizeof(const char*));
    ForcedMemcpy(reinterpret_cast<void*>(0x530ACF), &NewTarget, sizeof(const char*));
    
}
//Original: MAPSMD%02d.MIX at 0x81C2EC
void RedirectMapsMD(const char* NewTarget)
{
    //4793DB -> 81C2EC
    //530A37 -> 81C2EC
    ForcedMemcpy(reinterpret_cast<void*>(0x4793DB), &NewTarget, sizeof(const char*));
    ForcedMemcpy(reinterpret_cast<void*>(0x530A37), &NewTarget, sizeof(const char*));
}


//Original: EXPANDMD%02d.MIX at 0x82668C
void RedirectExpandMD()
{
    RedirectExpandMD(reinterpret_cast<const char*>(0x82668C));
}
//Original: ELOCAL*.MIX at 0x826614
void RedirectELocal()
{
    RedirectELocal(reinterpret_cast<const char*>(0x826614));
}
//Original: ECACHE*.MIX at 0x826620
void RedirectECache()
{
    RedirectECache(reinterpret_cast<const char*>(0x826620));
}
//Original: MOVIES%02d.MIX at 0x81C200
void RedirectMovies()
{
    RedirectMovies(reinterpret_cast<const char*>(0x81C200));
}
//Original: MOVMD%02d.MIX at 0x81C210
void RedirectMovMD()
{
    RedirectMovMD(reinterpret_cast<const char*>(0x81C210));
}
//Original: MAPS%02d.MIX at 0x81C2C4
void RedirectMaps()
{
    RedirectMaps(reinterpret_cast<const char*>(0x81C2C4));
}
//Original: MAPSMD%02d.MIX at 0x81C2EC
void RedirectMapsMD()
{
    RedirectMapsMD(reinterpret_cast<const char*>(0x81C2EC));
}





namespace InitialLoad
{
    unsigned int RandomUINT();
}

uint32_t Random_uint32()
{
    return InitialLoad::RandomUINT();
}
int32_t Random_int32()
{
    return static_cast<int32_t>(InitialLoad::RandomUINT());
}
uint16_t Random_uint16()
{
    return static_cast<uint16_t>(InitialLoad::RandomUINT());
}
int16_t Random_int16()
{
    return static_cast<int16_t>(InitialLoad::RandomUINT());
}
uint64_t Random_uint64()
{
    uint64_t L{ static_cast<uint64_t>(InitialLoad::RandomUINT()) }, H{ static_cast<uint64_t>(InitialLoad::RandomUINT()) };
    return (L << 32) | H;
}
int64_t Random_int64()
{
    return static_cast<int64_t>(Random_uint64());
}

template<typename T>
struct FakeEng
{
    using result_type = T;
    T(*pFn)();
    FakeEng(T(*pfn)()) :pFn(pfn) {};
    static constexpr result_type min() { return std::numeric_limits<T>().min(); }
    static constexpr result_type max() { return std::numeric_limits<T>().max(); }
    result_type operator()() { return pFn(); }
};

uint32_t Random_uint32(uint32_t Min, uint32_t Max)
{
    auto eng = FakeEng<uint32_t>((uint32_t(*)())Random_uint32);
    return std::uniform_int_distribution<uint32_t>(Min, Max)(eng);
}
int32_t Random_int32(int32_t Min, int32_t Max)
{
    return static_cast<int32_t>(Random_uint32(static_cast<uint32_t>(Min), static_cast<uint32_t>(Max)));
}
uint16_t Random_uint16(uint16_t Min, uint16_t Max)
{
    auto eng = FakeEng<uint16_t>((uint16_t(*)())Random_uint16);
    return std::uniform_int_distribution<uint16_t>(Min, Max)(eng);
}
int16_t Random_int16(int16_t Min, int16_t Max)
{
    return static_cast<int16_t>(Random_uint16(static_cast<uint16_t>(Min), static_cast<uint16_t>(Max)));
}
uint64_t Random_uint64(uint64_t Min, uint64_t Max)
{
    auto eng = FakeEng<uint64_t>((uint64_t(*)())Random_uint64);
    return std::uniform_int_distribution<uint64_t>(Min, Max)(eng);
}
int64_t Random_int64(int64_t Min, int64_t Max)
{
    return static_cast<int64_t>(Random_uint64(static_cast<uint64_t>(Min), static_cast<uint64_t>(Max)));
}
float Random_float(float Min, float Max)
{
    auto eng = FakeEng<uint32_t>((uint32_t(*)())Random_uint32);
    return std::uniform_real_distribution<float>(Min, Max)(eng);
}
double Random_double(double Min, double Max)
{
    auto eng = FakeEng<uint32_t>((uint32_t(*)())Random_uint32);
    return std::uniform_real_distribution<double>(Min, Max)(eng);
}

std::string RandStr(int i)
{
    static const char Pool[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
    std::string ss; ss.reserve(i + 1);
    for (int p = 0; p < i; p++)ss.push_back(Pool[Random_uint32(0, 61)]);
    return ss;
}

std::wstring RandWStr(int i)
{
    static const wchar_t Pool[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
    std::wstring ss; ss.reserve(i + 1);
    for (int p = 0; p < i; p++)ss.push_back(Pool[Random_uint32(0, 61)]);
    return ss;
}











std::wstring GBKToUTF16(const std::string& GBK)
{
    int nLength = MultiByteToWideChar(CP_ACP, 0, GBK.c_str(), -1, NULL, NULL);   // 获取缓冲区长度，再分配内存
    WCHAR* tch = new WCHAR[nLength + 4]();
    nLength = MultiByteToWideChar(CP_ACP, 0, GBK.c_str(), -1, tch, nLength);     // 将GBK转换成UTF16
    std::wstring ret = tch;
    delete[] tch;
    return ret;
}
std::wstring UTF8ToUTF16(const std::string& UTF8)
{
    int nLength = MultiByteToWideChar(CP_UTF8, 0, UTF8.c_str(), -1, NULL, NULL);   // 获取缓冲区长度，再分配内存
    WCHAR* tch = new WCHAR[nLength + 4]{};
    MultiByteToWideChar(CP_UTF8, 0, UTF8.c_str(), -1, tch, nLength);     // 将UTF-8转换成UTF16
    std::wstring ret = tch;
    delete[] tch;
    return ret;
}
std::string UTF16ToUTF8(const std::wstring& UTF16)
{
    int UTF8len = WideCharToMultiByte(CP_UTF8, 0, UTF16.c_str(), -1, 0, 0, 0, 0);// 获取UTF-8编码长度
    char* UTF8 = new CHAR[UTF8len + 4]{};
    WideCharToMultiByte(CP_UTF8, 0, UTF16.c_str(), -1, UTF8, UTF8len, 0, 0); //转换成UTF-8编码
    std::string ret = UTF8;
    delete[] UTF8;
    return ret;
}
std::string UTF16ToGBK(const std::wstring& UTF16)
{
    int GBKlen = WideCharToMultiByte(CP_ACP, 0, UTF16.c_str(), -1, 0, 0, 0, 0);// 获取UTF-8编码长度
    char* GBK = new CHAR[GBKlen + 4]{};
    WideCharToMultiByte(CP_ACP, 0, UTF16.c_str(), -1, GBK, GBKlen, 0, 0); //转换成UTF-8编码
    std::string ret = GBK;
    delete[] GBK;
    return ret;
}
std::string GBKToUTF8(const std::string& GBK)
{
    return UTF16ToUTF8(GBKToUTF16(GBK));
}
std::string UTF8ToGBK(const std::string& GBK)
{
    return UTF16ToGBK(UTF8ToUTF16(GBK));
}





size_t ECUpperHash::operator()(const std::string& s) const
{
    return static_cast<size_t>(SyringeData::SyringeHashUpper(s.c_str()));
}
bool ECUpperEqualPred::operator()(const std::string& s1, const std::string& s2) const
{
    auto E = ECUpperHash{};
    return E(s1) == E(s2);
}




void ForcedMemcpy(void* Target, const void* Source, size_t Size)
{
    DWORD protect_flag;
    VirtualProtect(Target, Size, PAGE_EXECUTE_READWRITE, &protect_flag);
    memcpy(Target, Source, Size);
    VirtualProtect(Target, Size, protect_flag, NULL);
}
void ForcedMemcpy(size_t Target, size_t Source, size_t Size)
{
    ForcedMemcpy(reinterpret_cast<void*>(Target), reinterpret_cast<const void*>(Source), Size);
}
bool ReadMemory(const void* Source, void* Buffer, size_t Size)
{
    return (ReadProcessMemory(GetCurrentProcess(), Source, Buffer, Size, nullptr) != FALSE);
}