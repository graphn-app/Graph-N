#pragma once

namespace base
{

/////////////////////////////////////////////
/////   GlobalID        /////////////////////
/////////////////////////////////////////////

class GlobalID final
{
public:
    static std::string uuid();
    static std::string client_id();
    static uint64_t random();
    static int64_t random_widget(int64_t min, int64_t max);
    static int64_t random_seed();

private:
    GlobalID();
    ~GlobalID() noexcept = default;

    NO_COPY_MOVE(GlobalID);

    static GlobalID& instance()
    {
        static GlobalID inst;
        return inst;
    }

private:
    std::mt19937_64 m_gen;
    std::mutex m_lock;
    std::once_flag m_flag;
    std::string m_client_id;
};

/////////////////////////////////////////////
/////   HashID          /////////////////////
/////////////////////////////////////////////

struct HashID
{
    HashID(const std::string& unique);
    virtual ~HashID() noexcept = 0;

    NO_COPY_MOVE(HashID);

    hid get_hash() const noexcept { return m_hash; }

    static hid make_hash(const std::string& unique) noexcept;

private:
    void set_hash(hid hash) noexcept;

private:
    hid m_hash = 0;
};

}
