
#pragma once

namespace ui
{
class PinInstance;
}

namespace data
{

struct Link final : public base::InstanceID
{
public:
    ~Link() override;

    static bool can_link(const ui::PinInstance* p1, const ui::PinInstance* p2);
    static Link* link(const ui::PinInstance* p1, const ui::PinInstance* p2);

    void unlink();

private:
    Link();

    const ui::PinInstance* input = nullptr;
    const ui::PinInstance* output = nullptr;

    hid input_id = 0;
    hid output_id = 0;

    bool validation_state = false;
};

} // namespace data
