#pragma once

template <typename T>
struct ObjectFlags
{
    static_assert(std::is_enum_v<T>, "T must be an enum type");
    using U = std::underlying_type_t<T>;

    constexpr ObjectFlags() noexcept = default;

    // variadic constructor for compile-time initialization
    // combines passed enum values using bitwise OR
    // example:
    //          ObjectFlags<MyEnum> obj(MyEnum::A, MyEnum::B)
    template <typename... Args>
    constexpr ObjectFlags(Args... args) noexcept
    {
        static_assert((std::is_same_v<T, Args> && ...), "Type mismatch: wrong enum passed");
        flags = (static_cast<U>(args) | ... | static_cast<U>(0));
    }

    ObjectFlags& operator=(const ObjectFlags other) { flags = other.flags; return *this; }

    // checks if a single target flag matches ANY flag in the provided list
    // example:
    //          is_one_of(MyEnum::A, MyEnum::A, MyEnum::B) -> true
    //          is_one_of(MyEnum::A, MyEnum::B, MyEnum::C) -> false
    template <typename... Args>
    [[nodiscard]] static constexpr bool is_one_of(T flag, Args... allowed_flags) noexcept 
    {
        static_assert((std::is_same_v<T, Args> && ...), "Type mismatch: wrong enum passed");
        return ((flag == allowed_flags) || ...);
    }

    // checks if specified flag is currently set
    // example:
    //          obj.has(MyEnum::A) -> true ONLY if A is set
    [[nodiscard]] constexpr bool has(T flag) const noexcept 
    { 
        const U mask = static_cast<U>(flag);
        return (flags & mask) == mask; 
    }

    // checks if specified flag is currently NOT set
    // example:
    //          obj.has_not(MyEnum::A) -> true ONLY if A is NOT set
    [[nodiscard]] constexpr bool has_not(T flag) const noexcept 
    { 
        const U mask = static_cast<U>(flag);
        return (flags & mask) == 0;
    }

    // checks if ALL specified flags are currently set (A && B && C)
    // example:
    //          obj.has(MyEnum::A, MyEnum::B) -> true ONLY if BOTH A and B are set
    template <typename... Args>
    [[nodiscard]] constexpr bool has_all(Args... args) const noexcept 
    { 
        static_assert((std::is_same_v<T, Args> && ...), "Type mismatch: wrong enum passed");
        const U mask = (static_cast<U>(args) | ... | static_cast<U>(0));
        return (flags & mask) == mask; 
    }

    // checks if AT LEAST ONE of the specified flags is set (A || B || C)
    // example:
    //          obj.has_any(MyEnum::A, MyEnum::B) -> true if A is set, B is set, or both are set
    template <typename... Args>
    [[nodiscard]] constexpr bool has_any(Args... args) const noexcept 
    { 
        static_assert((std::is_same_v<T, Args> && ...), "Type mismatch: wrong enum passed");
        const U mask = (static_cast<U>(args) | ... | static_cast<U>(0));
        return (flags & mask) != 0; 
    }

    // checks if AT LEAST ONE of the flags is object
    [[nodiscard]] constexpr bool has_any_by_mask_obj(ObjectFlags mask_obj) const noexcept 
    { 
        const U mask = mask_obj.get_raw();
        return (flags & mask) != 0;
    }

    // checks if NONE of the specified flags are set (!A && !B && !C)
    // example:
    //          obj.has_not(MyEnum::A, MyEnum::B) -> true ONLY if BOTH A and B are strictly 0
    template <typename... Args>
    [[nodiscard]] constexpr bool has_none(Args... args) const noexcept 
    { 
        static_assert((std::is_same_v<T, Args> && ...), "Type mismatch: wrong enum passed");
        const U mask = (static_cast<U>(args) | ... | static_cast<U>(0));
        return (flags & mask) == 0;
    }

    // convenience method: checks if one specific flag is set AND another is strictly NOT set
    // example:
    //          obj.has_and_has_not(MyEnum::MustHave, MyEnum::MustNotHave)
    [[nodiscard]] constexpr bool has_and_has_not(T must_be, T must_not_be) const noexcept 
    { 
        return has_all(must_be) && has_none(must_not_be); 
    }

    // adds (sets to 1) the specified flags using bitwise OR
    // example:
    //          obj.add(MyEnum::A, MyEnum::B);
    template <typename... Args>
    constexpr void add(Args... args) noexcept 
    {
        static_assert((std::is_same_v<T, Args> && ...), "Type mismatch: wrong enum passed");
        flags |= (static_cast<U>(args) | ... | static_cast<U>(0));
    }

    // removes (sets to 0) the specified flags using bitwise AND NOT
    // example:
    //          obj.remove(MyEnum::A, MyEnum::B);
    template <typename... Args>
    constexpr void remove(Args... args) noexcept 
    {
        static_assert((std::is_same_v<T, Args> && ...), "Type mismatch: wrong enum passed");
        flags &= ~(static_cast<U>(args) | ... | static_cast<U>(0));
    }

    // toggles (inverts) the specified flags using bitwise XOR
    // example:
    //          obj.toggle(MyEnum::A); -> 1 becomes 0, 0 becomes 1.
    template <typename... Args>
    constexpr void toggle(Args... args) noexcept 
    {
        static_assert((std::is_same_v<T, Args> && ...), "Type mismatch: wrong enum passed");
        flags ^= (static_cast<U>(args) | ... | static_cast<U>(0));
    }

    // adds the flag if the condition is true
    // example:
    //          obj.add_if(MyEnum::Visible, is_visible);
    constexpr void add_if(T f, bool cond) noexcept 
    { 
        if (cond)
            add(f);
    }

    // removes the flag if the condition is true
    // example:
    //          obj.remove_if(MyEnum::Visible, !is_visible);
    constexpr void remove_if(T f, bool cond) noexcept 
    { 
        if (cond)
            remove(f);
    }

    // adds the flag if the condition is true, otherwise removes it
    // example:
    //          obj.add_remove_if(MyEnum::Visible, is_visible);
    constexpr void add_remove_if(T f, bool cond) noexcept 
    { 
        if (cond) add(f);
        else remove(f); 
    }

    // adds ONLY the specific subset of flags from another instance, leaving other bits intact
    // example:
    //          obj.copy_from_by_mask(other_obj, MyEnum::A, MyEnum::B);
    template <typename... Args>
    constexpr void copy_from_by_mask_flags(ObjectFlags source, Args... mask_args) noexcept 
    {
        static_assert((std::is_same_v<T, Args> && ...), "Type mismatch: wrong enum passed");
        const U mask = (static_cast<U>(mask_args) | ... | static_cast<U>(0));
        flags = (flags & ~mask) | (source.get_raw() & mask);
    }

    // adds a subset of flags from another instance using a predefined ObjectFlags mask
    // example:
    //          obj.copy_from_by_mask(other_obj, g_node_inst_all_modes_mask);
    template <typename U1, typename U2>
    constexpr void copy_from_by_mask_obj(ObjectFlags<U1> source, ObjectFlags<U2> mask_flags) noexcept 
    {
        static_assert(std::is_same_v<T, U1>, "Type mismatch: source must use the same enum type");
        static_assert(std::is_same_v<T, U2>, "Type mismatch: mask must use the same enum type");
        flags = (flags & ~mask_flags.get_raw()) | (source.get_raw() & mask_flags.get_raw());
    }

    // returns the raw underlying integer representation of all flags
    [[nodiscard]] constexpr U get_raw() const noexcept { return flags; }

    // returns the raw underlying bits, filtered by the specified flags
    // example:
    //          uint64_t masked_bits = obj.get_by_mask(MyEnum::A, MyEnum::B);
    template <typename... Args>
    [[nodiscard]] constexpr U get_by_mask(Args... mask_args) const noexcept 
    {
        static_assert((std::is_same_v<T, Args> && ...), "Type mismatch: wrong enum passed");
        const U mask = (static_cast<U>(mask_args) | ... | static_cast<U>(0));
        return flags & mask;
    }

    // returns the raw underlying bits, filtered by a predefined ObjectFlags mask
    template <typename U1>
    [[nodiscard]] constexpr U get_by_mask(ObjectFlags<U1> mask_flags) const noexcept 
    {
        static_assert(std::is_same_v<T, U1>, "Type mismatch: mask must use the same enum type");
        return flags & mask_flags.get_raw();
    }

    // completely overwrites the current state with only the specified flags
    // example:
    //          obj.set(MyEnum::A); -> ONLY A will be 1, everything else becomes 0
    template <typename... Args>
    constexpr void set(Args... args) noexcept 
    {
        static_assert((std::is_same_v<T, Args> && ...), "Type mismatch: wrong enum passed");
        flags = (static_cast<U>(args) | ... | static_cast<U>(0));
    }

    // overwrites all flags directly using a raw integer value (e.g. from deserialization)
    constexpr void set_raw(U raw_flags) noexcept { flags = raw_flags; }

    // adds flags directly using a raw integer mask
    constexpr void add_raw(U raw_flags) noexcept { flags |= raw_flags; }

    // removes flags directly using a raw integer mask
    constexpr void remove_raw(U raw_flags) noexcept { flags &= ~raw_flags; }

    // clears all flags (sets to 0)
    constexpr void clear() noexcept { flags = 0; }

private:
    U flags = 0;
};

enum class NodeInstFlags : uint64_t
{
    None                      = 0,            // 0                  -  0x00
    Group                     = 1ULL << 0,    // 1                  -  0x01
    CanBeDeleted              = 1ULL << 1,    // 2                  -  0x02               // default
    CNI                       = 1ULL << 2,    // 4                  -  0x04
    Reroute                   = 1ULL << 3,    // 8                  -  0x08
    RerouteSplitted           = 1ULL << 4,    // 16                 -  0x10
    RerouteSplitted_R         = 1ULL << 5,    // 32                 -  0x20
    Hidden                    = 1ULL << 6,    // 64                 -  0x40
    PosSet                    = 1ULL << 7,    // 128                -  0x80
    SizeSet                   = 1ULL << 8,    // 256                -  0x100
    IgnoreSizeOnFrameEnd      = 1ULL << 9,    // 512                -  0x200
    PinnedSet                 = 1ULL << 10,   // 1024               -  0x400
    ShowHidden                = 1ULL << 11,   // 2048               -  0x800
    Deserialization           = 1ULL << 12,   // 4096               -  0x1000
    Pinned                    = 1ULL << 13,   // 8192               -  0x2000
    Collapsed                 = 1ULL << 14,   // 16384              -  0x4000
    ModeOnEvent               = 1ULL << 16,   // 65536              -  0x10000
    ModeMute                  = 1ULL << 17,   // 131072             -  0x20000
    ModeOnTrigger             = 1ULL << 18,   // 262144             -  0x40000
    ModeBypass                = 1ULL << 19,   // 524288             -  0x80000
    HasInPins                 = 1ULL << 20,   // 1048576            -  0x100000
    HasOutPins                = 1ULL << 21,   // 2097152            -  0x200000
    HasWidgets                = 1ULL << 22,   // 4194304            -  0x400000
    JsonApiDontExport         = 1ULL << 23,   // 8388608            -  0x800000
    JsonCustomSerialization   = 1ULL << 24,   // 16777216           -  0x1000000
    BlobCustomSerialization   = 1ULL << 25,   // 33554432           -  0x2000000
    MaskEditorReady           = 1ULL << 26,   // 67108864           -  0x4000000
    PosEditorReady            = 1ULL << 27,   // 134217728          -  0x8000000
    MaskEditorRunning         = 1ULL << 28,   // 268435456          -  0x10000000
    PosEditorRunning          = 1ULL << 29,   // 536870912          -  0x20000000
    PrimitiveNode             = 1ULL << 30,   // 1073741824         -  0x40000000
};

inline constexpr ObjectFlags<NodeInstFlags> g_node_inst_all_modes_mask
{
    //NodeInstFlags::ModeAlways,
    NodeInstFlags::ModeOnEvent,
    NodeInstFlags::ModeMute,
    NodeInstFlags::ModeOnTrigger,
    NodeInstFlags::ModeBypass
};

inline constexpr ObjectFlags<NodeInstFlags> g_node_inst_seriallizable_flags
{
    NodeInstFlags::ModeOnEvent,
    NodeInstFlags::ModeMute,
    NodeInstFlags::ModeOnTrigger,
    NodeInstFlags::ModeBypass,
    NodeInstFlags::Group,
    NodeInstFlags::ShowHidden,
    NodeInstFlags::Collapsed,
    NodeInstFlags::Pinned
};

enum class NodeObjFlags : uint64_t
{
    None                   = 0,            // 0          -  0x00
    HasInput               = 1ULL << 0,    // 1          -  0x01
    HasOutput              = 1ULL << 1,    // 2          -  0x02
    OutputNode             = 1ULL << 2,    // 4          -  0x04
    Dummy                  = 1ULL << 3,    // 8          -  0x08
    CanBypass              = 1ULL << 4,    // 16         -  0x10               // default
    CanMute                = 1ULL << 5,    // 32         -  0x20               // default
    CanRename              = 1ULL << 6,    // 64         -  0x40               // default
    Experimental           = 1ULL << 7,    // 128        -  0x80
    Deprecated             = 1ULL << 8,    // 256        -  0x100
    ApiNode                = 1ULL << 9,    // 512        -  0x200
    CustomWidget           = 1ULL << 10,   // 1024       -  0x400              // this flag is must if any of CustomWdgt** is set
    CustomWdgtAudio        = 1ULL << 11,   // 2048       -  0x800
    CustomWdgtVideo        = 1ULL << 12,   // 4096       -  0x1000
    CustomWdgtImage        = 1ULL << 13,   // 8192       -  0x2000
    CustomWdgtText         = 1ULL << 14,   // 16384      -  0x4000
    CustomWdgtRestrictCfg  = 1ULL << 15,   // 32768      -  0x8000             // disables custom widget config in menu
    CustomWdgtUploadButton = 1ULL << 16,   // 65536      -  0x10000            // displays 'Upload' button
    CustomWdgtFromSource   = 1ULL << 17,   // 131072     -  0x20000            // displays media from input source
    CustomWdgtFromOutput   = 1ULL << 18,   // 262144     -  0x40000            // displays media/text from output
};

inline constexpr ObjectFlags<NodeObjFlags> g_node_obj_all_custom_widget_mask
{
    NodeObjFlags::CustomWdgtAudio,
    NodeObjFlags::CustomWdgtVideo,
    NodeObjFlags::CustomWdgtImage,
    NodeObjFlags::CustomWdgtText,
    NodeObjFlags::CustomWdgtUploadButton,
    NodeObjFlags::CustomWdgtFromSource,
    NodeObjFlags::CustomWdgtFromOutput
};

inline constexpr ObjectFlags<NodeObjFlags> g_node_obj_seriallizable_flags
{
};

enum class PinInstFlags : uint64_t
{
    None                      = 0,          // 0                  -  0x00
    Hidden                    = 1ULL << 0,  // 1                  -  0x01
    RerouteSplittingPin       = 1ULL << 1,  // 2                  -  0x02
    Multiline                 = 1ULL << 2,  // 4                  -  0x04
    MultilineSet              = 1ULL << 3,  // 8                  -  0x08
    ColorHSV                  = 1ULL << 4,  // 16                 -  0x10
    ColorHex                  = 1ULL << 5,  // 32                 -  0x20
    ColorRaw                  = 1ULL << 6,  // 64                 -  0x40
    ColorFloat                = 1ULL << 7,  // 128                -  0x80
};

inline constexpr ObjectFlags<PinInstFlags> g_pin_inst_serializable_flags
{
    PinInstFlags::Multiline,
    PinInstFlags::MultilineSet,
    PinInstFlags::ColorHSV,
    PinInstFlags::ColorHex,
    PinInstFlags::ColorRaw,
    PinInstFlags::ColorFloat,
};

enum class PinObjFlags : uint64_t
{
    None                      = 0,          // 0                  -  0x00
    Hidden                    = 1ULL << 0,  // 1                  -  0x01
    Input                     = 1ULL << 1,  // 2                  -  0x02                  // default for inputs
    Output                    = 1ULL << 2,  // 4                  -  0x04                  // default for outputs
    Required                  = 1ULL << 3,  // 8                  -  0x08
    Optional                  = 1ULL << 4,  // 16                 -  0x10
    CanLink                   = 1ULL << 5,  // 32                 -  0x20                  // default
    CanUnlink                 = 1ULL << 6,  // 64                 -  0x40                  // default
    CanRename                 = 1ULL << 7,  // 128                -  0x80                  // default
    Advanced                  = 1ULL << 8,  // 256                -  0x100
    InputCanBeWidget          = 1ULL << 9,  // 512                -  0x200                 // default for inputs
    ImageUpload               = 1ULL << 10, // 1024               -  0x400
    AudioUpload               = 1ULL << 11, // 2048               -  0x800
    VideoUpload               = 1ULL << 12, // 4096               -  0x1000
    FileUpload                = 1ULL << 13, // 8192               -  0x2000
    TypeAny                   = 1ULL << 14, // 16384              -  0x4000
    Multiline                 = 1ULL << 15, // 32768              -  0x8000
    ClrAlphaEnabled           = 1ULL << 16, // 65536              -  0x10000               // default for inputs
    CagRequire                = 1ULL << 17, // 131072             -  0x20000
    DenyMultilineSwitch       = 1ULL << 18, // 262144             -  0x40000
    OutputIsList              = 1ULL << 19, // 524288             -  0x80000
    ForceInput                = 1ULL << 20, // 1048576            -  0x100000
    DefInput                  = 1ULL << 21, // 2097152            -  0x200000             // TODO ??
    Lazy                      = 1ULL << 22, // 4194304            -  0x400000             // TODO ??
    Multiselect               = 1ULL << 23, // 8388608            -  0x800000
    DisplAsNumber             = 1ULL << 24, // 16777216           -  0x1000000
    DisplAsColor              = 1ULL << 25, // 33554432           -  0x2000000
    DisplAsSlider             = 1ULL << 26, // 67108864           -  0x4000000
    DynamicPrompts            = 1ULL << 27, // 134217728          -  0x8000000
};

inline constexpr ObjectFlags<PinObjFlags> g_pin_obj_serializable_flags
{
    PinObjFlags::CagRequire,
};
