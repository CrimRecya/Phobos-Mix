#pragma once

#include "cJSON.h"
#include<string>
#include<vector>
#include <unordered_map>

bool _IH_IsTrueString(const std::string& s);

std::vector<int> cJSON_GetVectorInt(cJSON* Array);
std::vector<uint8_t> cJSON_GetVectorBool(cJSON* Array);
std::vector<std::string> cJSON_GetVectorString(cJSON* Array);
std::vector<cJSON*> cJSON_GetVectorObject(cJSON* Array);
void cJson_SwapData(cJSON* A, cJSON* B);

enum class StrBoolType :size_t
{
    Str_true_false,
    Str_True_False,
    Str_TRUE_FALSE,
    Str_yes_no,
    Str_Yes_No,
    Str_YES_NO,
    Str_t_f,
    Str_T_F,
    Str_y_n,
    Str_Y_N,
    Str_1_0
};
std::string StrBoolImpl(bool Val, StrBoolType Type);
const char* CStrBoolImpl(bool Val, StrBoolType Type);

class JsonFile;

class JsonObject
{
private:
    cJSON* Object{ nullptr };
public:
    JsonObject(cJSON* _F) { Object = _F; }
    JsonObject() : Object(nullptr) {}

    inline cJSON* GetRaw() const { return Object; }

    inline bool Available() const { return Object != nullptr; }
    inline int GetType() const { return Object->type; }
    inline bool IsTypeNumber() const { return ((Object->type & 0xFF) == cJSON_Number); }
    inline bool IsTypeNull() const { return ((Object->type & 0xFF) == cJSON_NULL); }
    inline bool IsTypeBool() const { return ((Object->type & 0xFF) == cJSON_True) || ((Object->type & 0xFF) == cJSON_False); }
    inline bool IsTypeString() const { return ((Object->type & 0xFF) == cJSON_String); }
    inline bool IsTypeArray() const { return ((Object->type & 0xFF) == cJSON_Array); }
    inline bool IsTypeObject() const { return ((Object->type & 0xFF) == cJSON_Object); }

    inline bool IsPropReference() const { return ((Object->type & cJSON_IsReference) == cJSON_IsReference); }
    inline bool IsPropConstString() const { return ((Object->type & cJSON_StringIsConst) == cJSON_StringIsConst); }

    inline JsonObject GetChildItem() const { return Object->child; }
    inline JsonObject GetPrevItem() const { return Object->prev; }
    inline JsonObject GetNextItem() const { return Object->next; }
    inline std::string GetName() const { return Object->string; }
    inline JsonObject GetObjectItem(const std::string& Str) const;

    inline bool HasItem(const std::string& Str) const { return GetObjectItem(Str).Available(); }

    //请在判断HasItem == true 之后再使用!
    int ItemInt(const std::string& Str) const;
    double ItemDouble(const std::string& Str) const;
    std::string ItemString(const std::string& Str) const;
    const char* ItemCString(const std::string& Str) const;
    bool ItemBool(const std::string& Str) const;
    bool ItemStrBool(const std::string& Str) const;
    size_t ItemArraySize(const std::string& Str) const;
    std::vector<int> ItemArrayInt(const std::string& Str) const;
    std::vector<uint8_t> ItemArrayBool(const std::string& Str) const;
    std::vector<std::string> ItemArrayString(const std::string& Str) const;
    std::vector<JsonObject> ItemArrayObject(const std::string& Str) const;

    inline int GetInt() const { return Object->valueint; }
    inline double GetDouble() const { return Object->valuedouble; }
    inline std::string GetString() const { return Object->valuestring; }
    inline const char* GetCString() const { return Object->valuestring; }
    inline bool GetBool() const { return ((Object->type & 0xFF) == cJSON_True) ? true : false; }
    inline bool GetStrBool() const { return (Object->valuestring != nullptr && _IH_IsTrueString(Object->valuestring)) ? true : false; }
    inline size_t GetArraySize() const { return (size_t)cJSON_GetArraySize(Object); }
    inline JsonObject GetArrayItem(size_t N) const { return { cJSON_GetArrayItem(Object, N) }; }
    inline std::vector<int> GetArrayInt() const { return cJSON_GetVectorInt(Object); }
    inline std::vector<uint8_t> GetArrayBool() const { return cJSON_GetVectorBool(Object); }
    inline std::vector<std::string> GetArrayString() const { return cJSON_GetVectorString(Object); }
    inline bool IsEmptyArray() { return IsTypeArray() && !GetChildItem().Available(); }
    inline bool IsEmptyObject() { return IsTypeObject() && !GetChildItem().Available(); }
    inline bool IsNotEmptyArray() { return IsTypeArray() && GetChildItem().Available(); }
    inline bool IsNotEmptyObject() { return IsTypeObject() && GetChildItem().Available(); }
    std::vector<JsonObject> GetArrayObject() const;
    std::unordered_map<std::string, JsonObject> GetMapObject() const;
    std::unordered_map<std::string, double> GetMapDouble() const;
    std::unordered_map<std::string, int> GetMapInt() const;
    std::unordered_map<std::string, bool> GetMapBool() const;
    std::unordered_map<std::string, std::string> GetMapString() const;
    std::vector < std::pair<std::string, JsonObject>> GetPVObject() const;
    std::vector < std::pair<std::string, double>> GetPVDouble() const;
    std::vector < std::pair<std::string, int>> GetPVInt() const;
    std::vector < std::pair<std::string, bool>> GetPVBool() const;
    std::vector < std::pair<std::string, std::string>> GetPVString() const;

    std::string GetText() const;

    JsonObject CreateObjectItem(const std::string& Str) const;
    void AddObjectItem(const std::string& Str, JsonObject Child, bool NeedsCopy) const;
    void AddNull(const std::string& Str) const;
    void AddInt(const std::string& Str, int Val) const;
    void AddDouble(const std::string& Str, double Val) const;
    void AddString(const std::string& Str, const std::string& Val) const;
    void AddBool(const std::string& Str, bool Val) const;
    void AddStrBool(const std::string& Str, bool Val, StrBoolType Type) const;

    //返回原来的Obj
    JsonFile SwapNull() const;
    JsonFile SwapInt(int Val) const;
    JsonFile SwapDouble(double Val) const;
    JsonFile SwapString(const std::string& Val) const;
    JsonFile SwapBool(bool Val) const;
    JsonFile SwapStrBool(bool Val, StrBoolType Type) const;
    JsonFile CopyAndSwap(JsonObject Obj, bool Recurse) const;
    JsonFile RedirectAndSwap(JsonObject Obj);
    void SwapObject(JsonObject Obj) const;

    //给非空的Object
    void SetNull() const;
    void SetInt(int Val) const;
    void SetDouble(double Val) const;
    void SetString(const std::string& Val) const;
    void SetBool(bool Val) const;
    void SetStrBool(bool Val, StrBoolType Type) const;
    void CopyObject(JsonObject Obj, bool Recurse) const;
    void RedirectObject(JsonObject Obj);

    //给空节点的非const函数
    void CreateNull();
    void CreateInt(int Val);
    void CreateDouble(double Val);
    void CreateString(const std::string& Val);
    void CreateBool(bool Val);
    void CreateStrBool(bool Val, StrBoolType Type);
    void CreateCopy(JsonObject Obj, bool Recurse);

    // 不知道是否为空时请调用以下函数
    void SetOrCreateNull();
    void SetOrCreateInt(int Val);
    void SetOrCreateDouble(double Val);
    void SetOrCreateString(const std::string& Val);
    void SetOrCreateBool(bool Val);
    void SetOrCreateStrBool(bool Val, StrBoolType Type);
    void SetOrCreateCopy(JsonObject Obj, bool Recurse);

    JsonFile DetachObjectItem(const std::string& Str);
    JsonFile DetachArrayItem(int Index);
    void DeleteObjectItem(const std::string& Str) { cJSON_DeleteItemFromObject(Object, Str.c_str()); }
    void DeleteArrayItem(int Index) { cJSON_DeleteItemFromArray(Object, Index); }

    //改变当前Object，合入的Json本身不变，数据仅复制
    //不需要可用性检查
    void Merge(JsonObject Obj);
};
const JsonObject NullJsonObject{ nullptr };

class JsonFile
{
private:
    cJSON* File{ nullptr };
public:
    JsonFile() : File(nullptr) {}
    JsonFile(cJSON* _F) { File = _F; }
    JsonFile(JsonObject Obj) { File = Obj.GetRaw(); }
    ~JsonFile() { if (File != nullptr)cJSON_Delete(File); }

    operator JsonObject() const { return JsonObject(File); }
    JsonObject GetObj() const { return JsonObject(File); }

    cJSON* GetRaw() const { return File; }
    cJSON* Release() { auto _F = File; File = nullptr; return _F; }

    bool Available() const { return File != nullptr; }
    JsonFile Duplicate(bool Recurse) const { return cJSON_Duplicate(File, Recurse); }//是否递归
    void DuplicateFromObject(JsonObject Obj, bool Recurse) { Clear();  File = cJSON_Duplicate(Obj.GetRaw(), Recurse); }//是否递归
    void DuplicateFromObject(JsonFile Obj, bool Recurse) { Clear(); File = cJSON_Duplicate(Obj.GetRaw(), Recurse); }//是否递归
    void Clear() { if (File != nullptr)cJSON_Delete(File); File = nullptr; }

    void Parse(std::string Str);
    std::string ParseChecked(std::string Str, const std::string& ErrorTip, int* ErrorPosition = nullptr);//返回错误信息
    void ParseWithOpts(std::string Str, const char** ReturnParseEnd, int RequireNullTerminated);
    void ParseTmp(std::string&& Str);
    std::string ParseTmpChecked(std::string&& Str, const std::string& ErrorTip, int* ErrorPosition = nullptr);//返回错误信息
    void ParseTmpWithOpts(std::string&& Str, const char** ReturnParseEnd, int RequireNullTerminated);
#ifdef IHCore
    void ParseFromFile(const char* FileName);
    void ParseFromFileWithOpts(const char* FileName, int RequireNullTerminated);
#endif

    JsonFile(const JsonFile&) : File(cJSON_Duplicate(File, true)) {}
    JsonFile(JsonFile&& _File)  noexcept : File(_File.Release()) {}
};

inline const char* Json_GetErrorPtr() { return cJSON_GetErrorPtr(); }//不理他


#ifdef IHCore
#include "..\Common\DataStructure.h"
template<typename FileLoader>
std::string GetStringFromFile(const char* FileName)
{
    FileLoader File;
    if (!File.Open(FileName, FileLoader::GetReadSign()))return "";
    BytePointerArray Arr = File.ReadWholeFile(16);
    std::string LoadStr{ (char*)Arr.Data };
    Arr.Delete();
    return LoadStr;
}
#endif
