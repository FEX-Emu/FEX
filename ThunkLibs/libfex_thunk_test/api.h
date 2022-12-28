#pragma once

// Equivalent of VkStructureType
enum class StructType {
    Struct1,
    Struct2,
    Struct3,
};

// Equivalent of VkBaseInStructure
struct TestBaseStruct {
    StructType Type;
    const void* Next;
};

// Equivalent of e.g. VkImageCreateInfo
struct TestStruct1 {
    StructType Type; // StructType::Struct1
    const void* Next;
    char Data2;
    int Data1;
};

struct TestStruct2 {
    StructType Type;  // StructType::Struct2
    const void* Next;
    int Data1;
};

// Equivalent of e.g. VkCommandBuffer
struct TestHandle;

//// Equivalent of e.g. VkDeviceQueueCreateInfo (references an array of structs that also need custom repacking)
//struct TestStruct3 {
//    StructType Type;
//    const void* Next;
//    int NumChildren;
//    TestStruct1* Children; // array of NumChildren elements
//};

extern "C" void TestFunction(TestStruct1*);
extern "C" void TestFunction2(TestHandle*);
//void TestFunctionStruct3(TestStruct3*);

// TODO: Test unaligned struct members (see https://godbolt.org/z/adcvq1E5d)
