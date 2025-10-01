#pragma once

#include <string>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <functional>


//下面的Redirect系列是更改了原有请求位置的请求目标
//原位的字符串并未更改，直接访问仍为原值
//由于确认资源完整性的时候请求的不完全是这些东西
//所以说是否需要相应修改来防止无法起始待确定

//要求传入的const char*是非临时值，生命期一直保持

//Original: EXPANDMD%02d.MIX at 0x82668C
void RedirectExpandMD(const char* NewTarget);
//Original: ELOCAL*.MIX at 0x826614
void RedirectELocal(const char* NewTarget);
//Original: ECACHE*.MIX at 0x826620
void RedirectECache(const char* NewTarget);
//Original: MOVIES%02d.MIX at 0x81C200
void RedirectMovies(const char* NewTarget);
//Original: MOVMD%02d.MIX at 0x81C210
void RedirectMovMD(const char* NewTarget);
//Original: MAPS%02d.MIX at 0x81C2C4
void RedirectMaps(const char* NewTarget);
//Original: MAPSMD%02d.MIX at 0x81C2EC
void RedirectMapsMD(const char* NewTarget);

//Original: EXPANDMD%02d.MIX at 0x82668C
void RedirectExpandMD();
//Original: ELOCAL*.MIX at 0x826614
void RedirectELocal();
//Original: ECACHE*.MIX at 0x826620
void RedirectECache();
//Original: MOVIES%02d.MIX at 0x81C200
void RedirectMovies();
//Original: MOVMD%02d.MIX at 0x81C210
void RedirectMovMD();
//Original: MAPS%02d.MIX at 0x81C2C4
void RedirectMaps();
//Original: MAPSMD%02d.MIX at 0x81C2EC
void RedirectMapsMD();



//一点随机数，随时可以访问
//对局当中不要用

uint32_t Random_uint32();
int32_t Random_int32();
uint16_t Random_uint16();
int16_t Random_int16();
uint64_t Random_uint64();
int64_t Random_int64();

uint32_t Random_uint32(uint32_t Min, uint32_t Max);
int32_t Random_int32(int32_t Min, int32_t Max);
uint16_t Random_uint16(uint16_t Min, uint16_t Max);
int16_t Random_int16(int16_t Min, int16_t Max);
uint64_t Random_uint64(uint64_t Min, uint64_t Max);
int64_t Random_int64(int64_t Min, int64_t Max);
float Random_float(float Min, float Max);
double Random_double(double Min, double Max);

std::string RandStr(int i);
std::wstring RandWStr(int i);

//一点编码转换函数 包括 GBK UTF8 UTF16
std::wstring GBKToUTF16(const std::string& GBK);
std::wstring UTF8ToUTF16(const std::string& UTF8);
std::string UTF16ToUTF8(const std::wstring& UTF16);
std::string UTF16ToGBK(const std::wstring& UTF16);
std::string GBKToUTF8(const std::string& GBK);
std::string UTF8ToGBK(const std::string& GBK);


//大小写处理
struct ECUpperHash
{
    inline size_t operator()(const std::string& s) const;
};
struct ECUpperEqualPred
{
    inline bool operator ()(const std::string& s1, const std::string& s2) const;
};
//忽略大小写的Map
template<typename T>
using IgnoreCaseUnorderedMap = std::unordered_map<std::string, T, ECUpperHash, ECUpperEqualPred>;
//忽略大小写的Set
using IgnoreCaseUnorderedSet = std::unordered_set<std::string, ECUpperHash, ECUpperEqualPred>;


//更NB的强转，比如说用来把成员函数转换成一般指针
template<typename dst_type, typename src_type>
dst_type constexpr UnionCast(src_type src)
{
    union {
        src_type s;
        dst_type d;
    }u;
    u.s = src;
    return u.d;
}




//无视权限的内存写入（得是存在的内存）
void ForcedMemcpy(void* Target, const void* Source, size_t Size);
void ForcedMemcpy(size_t TargetAddr, size_t SourceAddr, size_t Size);

//比较安全的内存读入（Source可以是任意地址，如果是不可读会返回false）
bool ReadMemory(const void* Source, void* Buffer, size_t Size);