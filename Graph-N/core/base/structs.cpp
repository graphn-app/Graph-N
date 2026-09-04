#include "core/base/structs.h"

namespace mngr
{

///////////////////////////////////////////////////////////////////
///////      PinOptions           /////////////////////////////////
///////////////////////////////////////////////////////////////////

std::string PinOptions::serialize() const noexcept
{
    try
    {
        u_json j;
        j["flags"] = flags.get_raw();
        return j.dump();
    }
    catch (const std::exception& ex)
    {
        LogExcep("Pin serialization err: " << ex.what());
    }

    return {};
}

void PinOptions::deserialize(const std::string& opts) noexcept
{
    if (opts.empty()) [[likely]]
        return;

    try
    {
        const auto j = u_json::parse(opts);
        if (j.is_object())
        {
            if (j.contains("flags") && j["flags"].is_number_unsigned())
                flags.set_raw(j["flags"].get<uint64_t>());
        }
    }
    catch (const std::exception& ex)
    {
        LogExcep("Pin deserialization err: " << ex.what());
    }
}

///////////////////////////////////////////////////////////////////
///////      NodeOptions          /////////////////////////////////
///////////////////////////////////////////////////////////////////

std::string NodeOptions::serialize() const noexcept
{
    try
    {
        u_json j;
        j["flags"] = flags.get_raw();
        j["display_source"] = display_source;
        return j.dump();
    }
    catch (const std::exception& ex)
    {
        LogExcep("Node serialization err: " << ex.what());
    }

    return {};
}

void NodeOptions::deserialize(const std::string& opts) noexcept
{
    if (opts.empty()) [[likely]]
        return;

    try
    {
        const auto j = u_json::parse(opts);
        if (j.is_object())
        {
            if (j.contains("flags") && j["flags"].is_number_unsigned())
                flags.set_raw(j["flags"].get<uint64_t>());
            if (j.contains("display_source") && j["display_source"].is_string())
                display_source = j["display_source"].get<std::string>();
        }
    }
    catch (const std::exception& ex)
    {
        LogExcep("Node deserialization err: " << ex.what());
    }
}

///////////////////////////////////////////////////////////////////
///////      LabelsList           /////////////////////////////////
///////////////////////////////////////////////////////////////////

const Label* LabelsList::get_label(uint32_t label_id) const
{
    for (const auto& label : labels)
    {
        if (label.id == label_id)
        {
            return &label;
        }
    }
    return nullptr;
}

Label* LabelsList::get_label(uint32_t label_id)
{
    auto label = std::as_const(*this).get_label(label_id);
    return cast::remove_const<Label>(label);
}

uint32_t LabelsList::add_label(bool auto_label /*= false*/, std::string name /*= ""*/)
{
    if (name.empty())
        name = "Label";

    size_t label_idx = 2;
    for (auto it = labels.begin(); it != labels.end(); )
    {
        if (tools::str::compare_no_case(it->name, name))
        {
            name = fmt::format(FMT_STRING("{} ({})"), name, label_idx++);
            it = labels.begin();
            continue;
        }
        ++it;
    }

    Label label;
    label.auto_label = auto_label;
    label.name = name;
    label.id = ++last_label_id;
    labels.push_back(std::move(label));
    sort_labels();
    return last_label_id;
}

bool LabelsList::rename_label(uint32_t label_id, const std::string& new_name)
{
    auto label = get_label(label_id);
    LogRAssert(label, "Label " << label_id << " not found", false);

    auto dup_it = std::ranges::find_if(labels, [&new_name, label_id](const Label& label) { return label.id != label_id && tools::str::compare_no_case(new_name, label.name); });
    if (dup_it != labels.end())
        return false;

    label->name = new_name;
    sort_labels();
    return true;
}

bool LabelsList::has_item(uint32_t label_id, hid item) const
{
    const Label* plabel = get_label(label_id);
    LogRAssert(plabel, "Label " << label_id << " not found", false);
    return plabel->members.contains(item);
}

void LabelsList::add_item(uint32_t label_id, hid item)
{
    Label* plabel = get_label(label_id);
    LogRAssert(plabel, "Label " << label_id << " not found", );
    plabel->members.insert(item);
}

void LabelsList::remove_item(uint32_t label_id, hid item)
{
    Label* plabel = get_label(label_id);
    LogRAssert(plabel, "Label " << label_id << " not found", );
    plabel->members.erase(item);
}

void LabelsList::remove_item_for_all(hid item)
{
    for (auto& label : labels)
    {
        remove_item(label.id, item);
    }
}

void LabelsList::sort_labels()
{
    labels.sort([](const Label& l1, const Label& l2) { return l1.name < l2.name; });
}

std::string LabelsList::serialize() const noexcept
{
    try
    {
        u_json j;
        j["last_label_id"] = last_label_id;

        for (const auto& label : labels)
        {
            u_json jlabel;
            jlabel["id"] = label.id;
            jlabel["name"] = label.name;
            jlabel["auto"] = label.auto_label;
            jlabel["members"] = label.members;

            j["labels"].push_back(std::move(jlabel));
        }

        return j.dump();
    }
    catch (const std::exception& ex)
    {
        LogExcep("Labels serialization err: " << ex.what());
    }

    return {};
}

void LabelsList::deserialize(const std::string& opts) noexcept
{
    if (opts.empty())
        return;

    try
    {
        last_label_id = 0;
        const auto j = u_json::parse(opts);
        if (j.is_object())
        {
            if (j.contains("last_label_id") && j["last_label_id"].is_number_integer())
                last_label_id = j["last_label_id"];

            if (j.contains("labels") && j["labels"].is_array())
            {
                const auto& jarr = j["labels"];
                for (auto& jitem : jarr)
                {
                    if (jitem.is_object())
                    {
                        Label label;
                        if (jitem.contains("id") && jitem["id"].is_number_integer())
                            label.id = jitem["id"].get<uint32_t>();
                        if (jitem.contains("name") && jitem["name"].is_string())
                            label.name = jitem["name"].get<std::string>();
                        if (jitem.contains("auto") && jitem["auto"].is_boolean())
                            label.auto_label = jitem["auto"].get<bool>();
                        if (jitem.contains("members") && jitem["members"].is_array())
                            label.members = jitem["members"].get<std::set<hid>>();

                        if (label.id != 0 && !label.name.empty())
                        {
                            last_label_id = std::max(last_label_id, label.id);
                            labels.push_back(std::move(label));
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& ex)
    {
        LogExcep("Labels deserialization err: " << ex.what());
    }
}

///////////////////////////////////////////////////////////////////
///////      NodeListOptions      /////////////////////////////////
///////////////////////////////////////////////////////////////////

std::string StarredList::serialize() const noexcept
{
    try
    {
        u_json j;
        j["starred"] = starred;
        return j.dump();
    }
    catch (const std::exception& ex)
    {
        LogExcep("Starred serialization err: " << ex.what());
    }

    return {};
}

void StarredList::deserialize(const std::string& opts) noexcept
{
    if (opts.empty())
        return;

    try
    {
        const auto j = u_json::parse(opts);
        if (j.contains("starred") && j["starred"].is_array())
            starred = j["starred"].get<std::set<hid>>();
    }
    catch (const std::exception& ex)
    {
        LogExcep("Starred deserialization err: " << ex.what());
    }
}

} // namespace mngr
