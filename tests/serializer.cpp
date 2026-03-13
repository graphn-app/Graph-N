#include <gtest/gtest.h>
#include "core/managers/workflow_serializer.h"

TEST(WorkflowSerializerTest, BasicPrimitivesRoundTrip)
{
    mngr::BlobWr writer;

    writer.add(true);
    writer.add(false);
    writer.add(static_cast<byte>(255));
    writer.add(static_cast<int32_t>(-123456));
    writer.add(static_cast<uint32_t>(4000000000));
    writer.add(static_cast<int64_t>(-9000000000000000000));
    writer.add(static_cast<double>(3.1415926535));

    mngr::BlobRd reader(writer.get());

    EXPECT_EQ(reader.read_bool(), true);
    EXPECT_EQ(reader.read_bool(), false);
    EXPECT_EQ(reader.read_byte(), 255);
    EXPECT_EQ(reader.read_i32(), -123456);
    EXPECT_EQ(reader.read_u32(), 4000000000);
    EXPECT_EQ(reader.read_i64(), -9000000000000000000);
    EXPECT_DOUBLE_EQ(reader.read_double(), 3.1415926535);

    EXPECT_TRUE(reader.empty());
}

TEST(WorkflowSerializerTest, StringsAndEmptyStrings)
{
    mngr::BlobWr writer;

    std::string normal_str = "Hello, Workflow!";
    std::string empty_str = "";

    writer.add(normal_str);
    writer.add(empty_str);

    mngr::BlobRd reader(writer.get());

    EXPECT_EQ(reader.read_string(), normal_str);
    EXPECT_EQ(reader.read_string(), empty_str);
    EXPECT_TRUE(reader.empty());
}

TEST(WorkflowSerializerTest, FloatArrays)
{
    mngr::BlobWr writer;

    std::vector<float> data = {1.1f, 2.2f, -3.3f, 0.0f};
    writer.add(data.data(), data.size());

    mngr::BlobRd reader(writer.get());

    std::vector<float> result(data.size());
    reader.read_float(result.data(), result.size());

    for (size_t i = 0; i < data.size(); ++i)
    {
        EXPECT_FLOAT_EQ(result[i], data[i]);
    }

    EXPECT_TRUE(reader.empty());
}

TEST(WorkflowSerializerTest, PrimitiveVariant)
{
    mngr::BlobWr writer;

    primitive p_str = std::string("Variant String");
    primitive p_i64 = static_cast<int64_t>(-42);
    primitive p_mono = std::monostate{};

    writer.add(p_str);
    writer.add(p_i64);
    writer.add(p_mono);

    mngr::BlobRd reader(writer.get());

    primitive res1 = reader.read_primitive();
    ASSERT_TRUE(std::holds_alternative<std::string>(res1));
    EXPECT_EQ(std::get<std::string>(res1), "Variant String");

    primitive res2 = reader.read_primitive();
    ASSERT_TRUE(std::holds_alternative<int64_t>(res2));
    EXPECT_EQ(std::get<int64_t>(res2), -42);

    primitive res3 = reader.read_primitive();
    EXPECT_TRUE(std::holds_alternative<std::monostate>(res3));

    EXPECT_TRUE(reader.empty());
}

TEST(WorkflowSerializerTest, BufferUnderflowProtection)
{
    mngr::BlobWr writer;
    writer.add(static_cast<int32_t>(42));

    mngr::BlobRd reader(writer.get());

    EXPECT_EQ(reader.read_i32(), 42);

    EXPECT_THROW(reader.read_i32(), std::runtime_error);
    EXPECT_THROW(reader.read_double(), std::runtime_error);
}

TEST(WorkflowSerializerTest, CorruptedStringLength)
{
    mngr::BlobWr writer;
    writer.add(static_cast<uint64_t>(999999)); 

    mngr::BlobRd reader(writer.get());
    EXPECT_THROW(reader.read_string(), std::runtime_error);
}

TEST(WorkflowSerializerTest, NestedBlobRoundTrip)
{
    mngr::BlobWr inner;
    inner.add(static_cast<int32_t>(42));
    inner.add(std::string("I am hidden inside!"));

    mngr::BlobWr outer;
    outer.add(static_cast<double>(3.14));
    outer.add(inner);
    outer.add(static_cast<uint32_t>(777));

    mngr::BlobRd reader(outer.get());
    EXPECT_DOUBLE_EQ(reader.read_double(), 3.14);

    mngr::BlobRd inner_reader = reader.read_blob();

    EXPECT_EQ(inner_reader.read_i32(), 42);
    EXPECT_EQ(inner_reader.read_string(), "I am hidden inside!");
    EXPECT_TRUE(inner_reader.empty());

    EXPECT_EQ(reader.read_u32(), 777);
    EXPECT_TRUE(reader.empty());
}

TEST(WorkflowSerializerTest, EmptyNestedBlob)
{
    mngr::BlobWr inner;

    mngr::BlobWr outer;
    outer.add(static_cast<int32_t>(111));
    outer.add(inner);
    outer.add(static_cast<int32_t>(222));

    mngr::BlobRd reader(outer.get());

    EXPECT_EQ(reader.read_i32(), 111);

    mngr::BlobRd inner_reader = reader.read_blob();
    EXPECT_TRUE(inner_reader.empty());

    EXPECT_THROW(inner_reader.read_i32(), std::runtime_error);

    EXPECT_EQ(reader.read_i32(), 222);
    EXPECT_TRUE(reader.empty());
}

TEST(WorkflowSerializerTest, CorruptedNestedBlobLength)
{
    mngr::BlobWr outer;
    outer.add(static_cast<uint64_t>(9999999)); 

    mngr::BlobRd reader(outer.get());
    EXPECT_THROW(reader.read_blob(), std::runtime_error);
}

TEST(WorkflowSerializerTest, DeeplyNestedBlobs)
{
    mngr::BlobWr level3;
    level3.add(std::string("Core"));

    mngr::BlobWr level2;
    level2.add(level3);

    mngr::BlobWr level1;
    level1.add(level2);

    mngr::BlobRd reader1(level1.get());
    mngr::BlobRd reader2 = reader1.read_blob();
    mngr::BlobRd reader3 = reader2.read_blob();

    EXPECT_EQ(reader3.read_string(), "Core");
    EXPECT_TRUE(reader3.empty());
    EXPECT_TRUE(reader2.empty());
    EXPECT_TRUE(reader1.empty());
}

TEST(WorkflowSerializerTest, ImGuiTypes)
{
    mngr::BlobWr writer;

    ImVec2 vec2{1.5f, -2.5f};
    ImVec4 vec4{0.1f, 0.2f, 0.3f, 1.0f};
    ImRect rect{ {0.0f, 0.0f}, {100.0f, 200.0f} };
    ImU32 color = 0xFFAABBCC;

    writer.add(vec2);
    writer.add(vec4);
    writer.add(rect);
    writer.add(color);

    mngr::BlobRd reader(writer.get());

    ImVec2 read_v2 = reader.read_vec2();
    EXPECT_FLOAT_EQ(read_v2.x, 1.5f);
    EXPECT_FLOAT_EQ(read_v2.y, -2.5f);

    ImVec4 read_v4 = reader.read_vec4();
    EXPECT_FLOAT_EQ(read_v4.x, 0.1f);
    EXPECT_FLOAT_EQ(read_v4.y, 0.2f);
    EXPECT_FLOAT_EQ(read_v4.z, 0.3f);
    EXPECT_FLOAT_EQ(read_v4.w, 1.0f);

    ImRect read_rect = reader.read_rect();
    EXPECT_FLOAT_EQ(read_rect.Min.x, 0.0f);
    EXPECT_FLOAT_EQ(read_rect.Min.y, 0.0f);
    EXPECT_FLOAT_EQ(read_rect.Max.x, 100.0f);
    EXPECT_FLOAT_EQ(read_rect.Max.y, 200.0f);

    EXPECT_EQ(reader.read_clr(), 0xFFAABBCC);

    EXPECT_TRUE(reader.empty());
}

TEST(WorkflowSerializerTest, CustomIDs)
{
    mngr::BlobWr writer;

    uint32_t my_iid = 1001;
    uint32_t my_wid = 2002;
    uint64_t my_hid = 999988887777;

    writer.add(my_iid);
    writer.add(my_wid);
    writer.add(my_hid);

    mngr::BlobRd reader(writer.get());

    iid read_iid = reader.read_iid();
    wid read_wid = reader.read_wid();
    hid read_hid = reader.read_hid();

    EXPECT_EQ(static_cast<uint32_t>(read_iid), 1001);
    EXPECT_EQ(static_cast<uint32_t>(read_wid), 2002);
    EXPECT_EQ(static_cast<uint64_t>(read_hid), 999988887777);

    EXPECT_TRUE(reader.empty());
}

TEST(BlobWrPatchTest, PatchValidOffset)
{
    mngr::BlobWr writer;

    writer.add(uint32_t(0));
    writer.add(int32_t(42));
    writer.add(byte(255));

    writer.patch(0, 1024);

    mngr::BlobRd reader(writer.get());
    EXPECT_EQ(reader.read_u32(), 1024);
    EXPECT_EQ(reader.read_i32(), 42);
    EXPECT_EQ(reader.read_byte(), 255);
}

TEST(BlobWrPatchTest, PatchThrowsOnEmptyBuffer)
{
    mngr::BlobWr writer;

    EXPECT_THROW(writer.patch(0, 100), std::runtime_error);
}

TEST(BlobWrPatchTest, PatchThrowsOnPartialOverlap)
{
    mngr::BlobWr writer;

    writer.add(uint32_t(0)); 
    writer.add(byte(1));

    EXPECT_THROW(writer.patch(2, 100), std::runtime_error);
}

TEST(BlobWrPatchTest, PatchThrowsOnOutOfBounds)
{
    mngr::BlobWr writer;
    writer.add(uint32_t(0)); 

    EXPECT_THROW(writer.patch(4, 100), std::runtime_error);
}

TEST(BlobWrPatchTest, BlobBlockIntegration)
{
    mngr::BlobWr writer;

    {
        mngr::BlobBlock block(writer); 

        writer.add(uint32_t(111));
        writer.add(uint32_t(222));
        writer.add(byte(99));
    }

    mngr::BlobRd reader(writer.get());

    EXPECT_EQ(reader.read_u32(), 9); 

    EXPECT_EQ(reader.read_u32(), 111);
    EXPECT_EQ(reader.read_u32(), 222);
    EXPECT_EQ(reader.read_byte(), 99);
}

TEST(BlobWrPatchTest, NestedReadBlobIntegration)
{
    mngr::BlobWr main_writer;

    {
        mngr::BlobBlock block(main_writer);
        main_writer.add(float(3.14f));
        main_writer.add(float(2.71f));
    }

    main_writer.add(uint64_t(9999));

    mngr::BlobRd main_reader(main_writer.get());

    {
        mngr::BlobRd nested_reader = main_reader.read_blob();
        EXPECT_EQ(nested_reader.read_float(), 3.14f);
    }

    EXPECT_EQ(main_reader.read_u64(), 9999);
}

TEST(BlobVersioningTest, ReadOldFormatWithNewCode)
{
    mngr::BlobWr old_file;

    {
        mngr::BlobBlock block(old_file);
        old_file.add(uint32_t(42));
        old_file.add(ImVec2(10.0f, 20.0f));
    }

    mngr::BlobRd reader(old_file.get());
    mngr::BlobRd node_reader = reader.read_blob();

    uint32_t id = node_reader.read_u32();
    ImVec2 pos = node_reader.read_vec2();

    ImU32 color = IM_COL32(0, 0, 0, 255);
    if (!node_reader.empty())
    {
        color = node_reader.read_clr();
    }
    else
    {
        color = IM_COL32(255, 255, 255, 255);
    }

    EXPECT_EQ(id, 42);
    EXPECT_EQ(pos.x, 10.0f);
    EXPECT_EQ(color, IM_COL32(255, 255, 255, 255));
}

TEST(BlobVersioningTest, ReadNewFormatWithOldCode)
{
    mngr::BlobWr new_file;

    {
        mngr::BlobBlock block(new_file);
        new_file.add(uint32_t(42));
        new_file.add(ImVec2(10.0f, 20.0f));
        new_file.add(IM_COL32(255, 0, 0, 255));
    }

    new_file.add(uint64_t(999));

    mngr::BlobRd reader(new_file.get());

    {
        mngr::BlobRd node_reader = reader.read_blob();

        uint32_t id = node_reader.read_u32();
        ImVec2 pos = node_reader.read_vec2();
    } 

    EXPECT_EQ(reader.read_u64(), 999);
}
