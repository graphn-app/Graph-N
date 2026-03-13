#include <gtest/gtest.h>
#include "tools/blob_serializer.h"

TEST(WorkflowSerializerTest, BasicPrimitivesRoundTrip)
{
    blob::Writer writer;
    
    writer.push(true);
    writer.push(false);
    writer.push(static_cast<uint8_t>(255));
    writer.push(static_cast<int32_t>(-123456));
    writer.push(static_cast<uint32_t>(4000000000));
    writer.push(static_cast<int64_t>(-9000000000000000000));
    writer.push(static_cast<double>(3.1415926535));

    blob::Reader reader(writer.get());

    EXPECT_EQ(reader.pop_bool(), true);
    EXPECT_EQ(reader.pop_bool(), false);
    EXPECT_EQ(reader.pop_u8(), 255);
    EXPECT_EQ(reader.pop_i32(), -123456);
    EXPECT_EQ(reader.pop_u32(), 4000000000);
    EXPECT_EQ(reader.pop_i64(), -9000000000000000000);
    EXPECT_DOUBLE_EQ(reader.pop_double(), 3.1415926535);

    EXPECT_TRUE(reader.empty());
}

TEST(WorkflowSerializerTest, StringsAndEmptyStrings)
{
    blob::Writer writer;

    std::string normal_str = "Hello, Workflow!";
    std::string empty_str = "";

    writer.push(normal_str);
    writer.push(empty_str);

    blob::Reader reader(writer.get());

    EXPECT_EQ(reader.pop_string(), normal_str);
    EXPECT_EQ(reader.pop_string(), empty_str);
    EXPECT_TRUE(reader.empty());
}

TEST(WorkflowSerializerTest, FloatArrays)
{
    blob::Writer writer;

    std::vector<float> data = {1.1f, 2.2f, -3.3f, 0.0f};
    writer.push(data.data(), data.size());

    blob::Reader reader(writer.get());

    std::vector<float> result(data.size());
    reader.pop_buff(result.data());

    for (size_t i = 0; i < data.size(); ++i)
    {
        EXPECT_FLOAT_EQ(result[i], data[i]);
    }

    EXPECT_TRUE(reader.empty());
}

TEST(WorkflowSerializerTest, PrimitiveVariant)
{
    blob::Writer writer;

    primitive p_str = std::string("Variant String");
    primitive p_i64 = static_cast<int64_t>(-42);
    primitive p_mono = std::monostate{};

    writer.push(p_str);
    writer.push(p_i64);
    writer.push(p_mono);

    blob::Reader reader(writer.get());

    primitive res1 = reader.pop_primitive();
    ASSERT_TRUE(std::holds_alternative<std::string>(res1));
    EXPECT_EQ(std::get<std::string>(res1), "Variant String");

    primitive res2 = reader.pop_primitive();
    ASSERT_TRUE(std::holds_alternative<int64_t>(res2));
    EXPECT_EQ(std::get<int64_t>(res2), -42);

    primitive res3 = reader.pop_primitive();
    EXPECT_TRUE(std::holds_alternative<std::monostate>(res3));

    EXPECT_TRUE(reader.empty());
}

TEST(WorkflowSerializerTest, BufferUnderflowProtection)
{
    blob::Writer writer;
    writer.push(static_cast<int32_t>(42));

    blob::Reader reader(writer.get());

    EXPECT_EQ(reader.pop_i32(), 42);

    EXPECT_THROW(reader.pop_i32(), std::runtime_error);
    EXPECT_THROW(reader.pop_double(), std::runtime_error);
}

TEST(WorkflowSerializerTest, CorruptedStringLength)
{
    blob::Writer writer;
    writer.push(static_cast<uint64_t>(999999)); 

    blob::Reader reader(writer.get());
    EXPECT_THROW(reader.pop_string(), std::runtime_error);
}

TEST(WorkflowSerializerTest, NestedBlobRoundTrip)
{
    blob::Writer inner;
    inner.push(static_cast<int32_t>(42));
    inner.push(std::string("I am hidden inside!"));

    blob::Writer outer;
    outer.push(static_cast<double>(3.14));
    outer.push(inner);
    outer.push(static_cast<uint32_t>(777));

    blob::Reader reader(outer.get());
    EXPECT_DOUBLE_EQ(reader.pop_double(), 3.14);

    blob::Reader inner_reader = reader.pop_block();

    EXPECT_EQ(inner_reader.pop_i32(), 42);
    EXPECT_EQ(inner_reader.pop_string(), "I am hidden inside!");
    EXPECT_TRUE(inner_reader.empty());

    EXPECT_EQ(reader.pop_u32(), 777);
    EXPECT_TRUE(reader.empty());
}

TEST(WorkflowSerializerTest, EmptyNestedBlob)
{
    blob::Writer inner;

    blob::Writer outer;
    outer.push(static_cast<int32_t>(111));
    outer.push(inner);
    outer.push(static_cast<int32_t>(222));

    blob::Reader reader(outer.get());

    EXPECT_EQ(reader.pop_i32(), 111);

    blob::Reader inner_reader = reader.pop_block();
    EXPECT_TRUE(inner_reader.empty());

    EXPECT_THROW(inner_reader.pop_i32(), std::runtime_error);

    EXPECT_EQ(reader.pop_i32(), 222);
    EXPECT_TRUE(reader.empty());
}

TEST(WorkflowSerializerTest, CorruptedNestedBlobLength)
{
    blob::Writer outer;
    outer.push(static_cast<uint64_t>(9999999)); 

    blob::Reader reader(outer.get());
    EXPECT_THROW(reader.pop_blob(), std::runtime_error);
}

TEST(WorkflowSerializerTest, DeeplyNestedBlobs)
{
    blob::Writer level3;
    level3.push(std::string("Core"));

    blob::Writer level2;
    level2.push(level3);

    blob::Writer level1;
    level1.push(level2);

    blob::Reader reader1(level1.get());
    blob::Reader reader2 = reader1.pop_block();
    blob::Reader reader3 = reader2.pop_block();

    EXPECT_EQ(reader3.pop_string(), "Core");
    EXPECT_TRUE(reader3.empty());
    EXPECT_TRUE(reader2.empty());
    EXPECT_TRUE(reader1.empty());
}

TEST(WorkflowSerializerTest, ImGuiTypes)
{
    blob::Writer writer;

    ImVec2 vec2{1.5f, -2.5f};
    ImVec4 vec4{0.1f, 0.2f, 0.3f, 1.0f};
    ImRect rect{ {0.0f, 0.0f}, {100.0f, 200.0f} };
    ImU32 color = 0xFFAABBCC;

    writer.push(vec2);
    writer.push(vec4);
    writer.push(rect);
    writer.push(color);

    blob::Reader reader(writer.get());

    ImVec2 read_v2 = reader.pop_vec2();
    EXPECT_FLOAT_EQ(read_v2.x, 1.5f);
    EXPECT_FLOAT_EQ(read_v2.y, -2.5f);

    ImVec4 read_v4 = reader.pop_vec4();
    EXPECT_FLOAT_EQ(read_v4.x, 0.1f);
    EXPECT_FLOAT_EQ(read_v4.y, 0.2f);
    EXPECT_FLOAT_EQ(read_v4.z, 0.3f);
    EXPECT_FLOAT_EQ(read_v4.w, 1.0f);

    ImRect read_rect = reader.pop_rect();
    EXPECT_FLOAT_EQ(read_rect.Min.x, 0.0f);
    EXPECT_FLOAT_EQ(read_rect.Min.y, 0.0f);
    EXPECT_FLOAT_EQ(read_rect.Max.x, 100.0f);
    EXPECT_FLOAT_EQ(read_rect.Max.y, 200.0f);

    EXPECT_EQ(reader.pop_clr(), 0xFFAABBCC);

    EXPECT_TRUE(reader.empty());
}

TEST(WorkflowSerializerTest, CustomIDs)
{
    blob::Writer writer;

    uint32_t my_iid = 1001;
    uint32_t my_wid = 2002;
    uint64_t my_hid = 999988887777;

    writer.push(my_iid);
    writer.push(my_wid);
    writer.push(my_hid);

    blob::Reader reader(writer.get());

    iid read_iid = reader.pop_iid();
    wid read_wid = reader.pop_wid();
    hid read_hid = reader.pop_hid();

    EXPECT_EQ(static_cast<uint32_t>(read_iid), 1001);
    EXPECT_EQ(static_cast<uint32_t>(read_wid), 2002);
    EXPECT_EQ(static_cast<uint64_t>(read_hid), 999988887777);

    EXPECT_TRUE(reader.empty());
}

TEST(BlobWrPatchTest, PatchValidOffset)
{
    blob::Writer writer;

    writer.push(uint32_t(0));
    writer.push(int32_t(42));
    writer.push(uint8_t(255));

    writer.patch(0, 1024);

    blob::Reader reader(writer.get());
    EXPECT_EQ(reader.pop_u32(), 1024);
    EXPECT_EQ(reader.pop_i32(), 42);
    EXPECT_EQ(reader.pop_u8(), 255);
}

TEST(BlobWrPatchTest, PatchThrowsOnEmptyBuffer)
{
    blob::Writer writer;

    EXPECT_THROW(writer.patch(0, 100), std::runtime_error);
}

TEST(BlobWrPatchTest, PatchThrowsOnPartialOverlap)
{
    blob::Writer writer;

    writer.push(uint32_t(0)); 
    writer.push(uint8_t(1));

    EXPECT_THROW(writer.patch(2, 100), std::runtime_error);
}

TEST(BlobWrPatchTest, PatchThrowsOnOutOfBounds)
{
    blob::Writer writer;
    writer.push(uint32_t(0)); 

    EXPECT_THROW(writer.patch(4, 100), std::runtime_error);
}

TEST(BlobWrPatchTest, BlobBlockIntegration)
{
    blob::Writer writer;

    {
        blob::Block block(writer); 

        writer.push(uint32_t(111));
        writer.push(uint32_t(222));
        writer.push(uint8_t(99));
    }

    blob::Reader reader(writer.get());

    EXPECT_EQ(reader.pop_u32(), 9); 

    EXPECT_EQ(reader.pop_u32(), 111);
    EXPECT_EQ(reader.pop_u32(), 222);
    EXPECT_EQ(reader.pop_u8(), 99);
}

TEST(BlobWrPatchTest, NestedReadBlobIntegration)
{
    blob::Writer main_writer;

    {
        blob::Block block(main_writer);
        main_writer.push(float(3.14f));
        main_writer.push(float(2.71f));
    }

    main_writer.push(uint64_t(9999));

    blob::Reader main_reader(main_writer.get());

    {
        blob::Reader nested_reader = main_reader.pop_block();
        EXPECT_EQ(nested_reader.pop_float(), 3.14f);
    }

    EXPECT_EQ(main_reader.pop_u64(), 9999);
}

TEST(BlobVersioningTest, ReadOldFormatWithNewCode)
{
    blob::Writer old_file;

    {
        blob::Block block(old_file);
        old_file.push(uint32_t(42));
        old_file.push(ImVec2(10.0f, 20.0f));
    }

    blob::Reader reader(old_file.get());
    blob::Reader node_reader = reader.pop_block();

    uint32_t id = node_reader.pop_u32();
    ImVec2 pos = node_reader.pop_vec2();

    ImU32 color = IM_COL32(0, 0, 0, 255);
    if (!node_reader.empty())
    {
        color = node_reader.pop_clr();
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
    blob::Writer new_file;

    {
        blob::Block block(new_file);
        new_file.push(uint32_t(42));
        new_file.push(ImVec2(10.0f, 20.0f));
        new_file.push(IM_COL32(255, 0, 0, 255));
    }

    new_file.push(uint64_t(999));

    blob::Reader reader(new_file.get());

    {
        blob::Reader node_reader = reader.pop_block();

        uint32_t id = node_reader.pop_u32();
        ImVec2 pos = node_reader.pop_vec2();
    } 

    EXPECT_EQ(reader.pop_u64(), 999);
}

TEST(WorkflowSerializerTest, BlobMoveAndEquality)
{
    blob::Writer w1;
    w1.push(static_cast<int32_t>(777));

    blob::Writer w2;
    w2.push(static_cast<int32_t>(777));

    blob::Blob b1 = w1.release();
    blob::Blob b2 = w2.release();

    EXPECT_TRUE(b1 == b2);
    EXPECT_FALSE(b1 != b2);

    b2->push_back(0xFF);
    EXPECT_TRUE(b1 != b2);

    blob::Blob b3 = std::move(b1);
    EXPECT_TRUE(b1->empty()); 
    EXPECT_EQ(b3->size(), sizeof(int32_t)); 
}

TEST(WorkflowSerializerTest, WriterResetAndRelease)
{
    blob::Writer writer;
    writer.push(static_cast<int32_t>(42));

    EXPECT_GT(writer.get()->size(), 0);

    writer.reset();
    EXPECT_TRUE(writer.get()->empty()); 

    writer.push(static_cast<uint8_t>(99));
    blob::Blob b = writer.release();

    EXPECT_TRUE(writer.get()->empty());
    EXPECT_EQ(b->size(), sizeof(uint8_t));
}

TEST(WorkflowSerializerTest, ReaderMoveSemantics)
{
    blob::Writer writer;
    writer.push(static_cast<int32_t>(111));
    writer.push(static_cast<int32_t>(222));

    blob::Reader r1(writer.get());
    EXPECT_EQ(r1.pop_i32(), 111);

    blob::Reader r2(std::move(r1));

    EXPECT_THROW(r1.pop_i32(), std::runtime_error); 

    EXPECT_EQ(r2.pop_i32(), 222);
    EXPECT_TRUE(r2.empty());
}

TEST(WorkflowSerializerTest, PushPopRawBlobDirectly)
{
    blob::Writer inner;
    inner.push(std::string("RawDataInside"));
    blob::Blob b = inner.release();

    blob::Writer outer;
    outer.push(static_cast<uint8_t>(1));
    outer.push(b); 
    outer.push(static_cast<uint8_t>(2));

    blob::Reader reader(outer.get());
    EXPECT_EQ(reader.pop_u8(), 1);

    blob::Blob read_b = reader.pop_blob();

    EXPECT_TRUE(b == read_b);

    blob::Reader inner_reader(read_b);
    EXPECT_EQ(inner_reader.pop_string(), "RawDataInside");

    EXPECT_EQ(reader.pop_u8(), 2);
    EXPECT_TRUE(reader.empty());
}

TEST(WorkflowSerializerTest, PopRawByteBuffers)
{
    blob::Writer writer;
    std::vector<byte> src_data = { 0x0A, 0x0B, 0x0C, 0x0D };
    writer.push(src_data.data(), src_data.size());

    blob::Reader reader(writer.get());

    std::vector<byte> dst_data(src_data.size());
    reader.pop_buff(dst_data.data());

    EXPECT_EQ(src_data, dst_data);
    EXPECT_TRUE(reader.empty());

    blob::Writer writer2;
    writer2.push(src_data.data(), src_data.size());
    blob::Reader reader2(writer2.get());

    byte_buff returned_buff = reader2.pop_buff();
    EXPECT_EQ(returned_buff.size(), src_data.size());
    EXPECT_EQ(returned_buff[0], 0x0A);
}

TEST(BlobBlockTest, DoubleBeginThrows)
{
    blob::Writer writer;
    blob::Block block(writer, true);

    EXPECT_THROW(block.begin(), std::runtime_error);
}

TEST(BlobBlockTest, DoubleEndThrows)
{
    blob::Writer writer;
    blob::Block block(writer, true);

    block.end(); 

    EXPECT_THROW(block.end(), std::runtime_error);
}

TEST(BlobBlockTest, AutoEndOnDestruction)
{
    blob::Writer writer;
    {
        blob::Block block(writer); 
        writer.push(static_cast<int32_t>(123));
    }

    blob::Reader reader(writer.get());
    blob::Reader nested = reader.pop_block();
    EXPECT_EQ(nested.pop_i32(), 123);
    EXPECT_TRUE(nested.empty());
}

TEST(BlobBlockTest, ReusabilityExplicitBeginEnd)
{
    blob::Writer writer;
    blob::Block block(writer, false); 

    block.begin();
    writer.push(static_cast<int32_t>(111));
    block.end();

    block.begin();
    writer.push(static_cast<int32_t>(222));
    writer.push(static_cast<int32_t>(333));
    block.end();

    blob::Reader reader(writer.get());

    blob::Reader block1 = reader.pop_block();
    EXPECT_EQ(block1.pop_i32(), 111);
    EXPECT_TRUE(block1.empty());

    blob::Reader block2 = reader.pop_block();
    EXPECT_EQ(block2.pop_i32(), 222);
    EXPECT_EQ(block2.pop_i32(), 333);
    EXPECT_TRUE(block2.empty());

    EXPECT_TRUE(reader.empty());
}

TEST(BlobBlockTest, ReusabilityMixedAutoAndManual)
{
    blob::Writer writer;

    {
        blob::Block block(writer);

        writer.push(std::string("First"));
        block.end();

        block.begin(); 
        writer.push(std::string("Second"));
    }

    blob::Reader reader(writer.get());

    blob::Reader block1 = reader.pop_block();
    EXPECT_EQ(block1.pop_string(), "First");
    EXPECT_TRUE(block1.empty());

    blob::Reader block2 = reader.pop_block();
    EXPECT_EQ(block2.pop_string(), "Second");
    EXPECT_TRUE(block2.empty());

    EXPECT_TRUE(reader.empty());
}
