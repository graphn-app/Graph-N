/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

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
    None                           = 0,
    Group                          = 1ULL << 0,
    CanBeDeleted                   = 1ULL << 1,    // default
    CNI                            = 1ULL << 2,
    __AVAILABLE__1                 = 1ULL << 3,
    RerouteSplitted                = 1ULL << 4,
    RerouteSplitted_R              = 1ULL << 5,
    Hidden                         = 1ULL << 6,
    PosSet                         = 1ULL << 7,
    SizeSet                        = 1ULL << 8,
    IgnoreSizeOnFrameEnd           = 1ULL << 9,
    PinnedSet                      = 1ULL << 10,
    ShowHidden                     = 1ULL << 11,
    Deserialization                = 1ULL << 12,
    Pinned                         = 1ULL << 13,
    Collapsed                      = 1ULL << 14,
    ModeOnEvent                    = 1ULL << 16,
    ModeMute                       = 1ULL << 17,
    ModeOnTrigger                  = 1ULL << 18,
    ModeBypass                     = 1ULL << 19,
    HasInPins                      = 1ULL << 20,
    HasOutPins                     = 1ULL << 21,
    HasWidgets                     = 1ULL << 22,
    JsonApiDontExport              = 1ULL << 23,
    JsonCustomSerialization        = 1ULL << 24,
    JsonCustomSerltnWidgetsOnly    = 1ULL << 25,
    BlobCustomSerialization        = 1ULL << 26,
    BlobCustomSerltnWidgetsOnly    = 1ULL << 27,
    MaskEditorReady                = 1ULL << 28,
    PosEditorReady                 = 1ULL << 29,
    MaskEditorRunning              = 1ULL << 30,
    PosEditorRunning               = 1ULL << 31,
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
    None                        = 0,
    HasInput                    = 1ULL << 0,
    HasOutput                   = 1ULL << 1,
    OutputNode                  = 1ULL << 2,
    Dummy                       = 1ULL << 3,
    CanBypass                   = 1ULL << 4,
    CanMute                     = 1ULL << 5,
    CanRename                   = 1ULL << 6,
    Experimental                = 1ULL << 7,
    Deprecated                  = 1ULL << 8,
    ApiNode                     = 1ULL << 9,
    CustomMediaWidget           = 1ULL << 10,   // this flag is must if any of CustomWdgt** is set
    CustomMediaAudio            = 1ULL << 11,
    CustomMediaVideo            = 1ULL << 12,
    CustomMediaImage            = 1ULL << 13,
    CustomMediaText             = 1ULL << 14,
    RestrictCfg                 = 1ULL << 15,   // disables custom widget config in menu
    CustomMediaUploadButton     = 1ULL << 16,   // displays 'Upload' button
    CustomMediaRefreshButton    = 1ULL << 17,   // displays 'Refresh' button
    CustomMediaFromSourceIn     = 1ULL << 18,   // displays media from input source
    CustomMediaFromSourceOut    = 1ULL << 19,   // displays media from output output
    CustomMediaFromOutput       = 1ULL << 20,   // displays media/text from output
    DevOnly                     = 1ULL << 21,
    SpecNote                    = 1ULL << 22,   // Note
    SpecMdNote                  = 1ULL << 23,   // MarkdownNote
    SpecPrimitive               = 1ULL << 24,   // PrimitiveNode
    SpecReroute                 = 1ULL << 25,   // Reroute, ReroutePrimitive|pysssss, Reroute (rgthree)
    SpecFastBypasserRgthree     = 1ULL << 26,   // Fast Bypasser (rgthree)
    SpecFastMuterRgthree        = 1ULL << 27,   // Fast Muter (rgthree)
    SpecFastGMuterRgthree       = 1ULL << 28,   // Fast Groups Muter (rgthree)
    SpecFastGBypasserRgthree    = 1ULL << 29,   // Fast Groups Bypasser (rgthree)
    SpecFastActRgthree          = 1ULL << 30,   // Fast Actions Button (rgthree)
    SpecBookmarkRgthree         = 1ULL << 31,   // Bookmark
    CustomJsNode                = 1ULL << 32,   // node has custom JS logic and need to be re-implemented
    SpecSwitherRgthree          = 1ULL << 33,   
    NoHistoryOnPinValChange     = 1ULL << 34,   
    NoPinCagMenu                = 1ULL << 35,   
    CustomMediaBase64           = 1ULL << 36,   
};

inline constexpr ObjectFlags<NodeObjFlags> g_node_obj_all_custom_widget_mask
{
    NodeObjFlags::CustomMediaAudio,
    NodeObjFlags::CustomMediaVideo,
    NodeObjFlags::CustomMediaImage,
    NodeObjFlags::CustomMediaText,
    NodeObjFlags::CustomMediaUploadButton,
    NodeObjFlags::CustomMediaRefreshButton,
    NodeObjFlags::CustomMediaBase64,
    NodeObjFlags::CustomMediaFromSourceIn,
    NodeObjFlags::CustomMediaFromSourceOut,
    NodeObjFlags::CustomMediaFromOutput
};

inline constexpr ObjectFlags<NodeObjFlags> g_node_obj_serializable_flags
{
    NodeObjFlags::CustomMediaWidget,
    NodeObjFlags::CustomMediaAudio,
    NodeObjFlags::CustomMediaVideo,
    NodeObjFlags::CustomMediaImage,
    NodeObjFlags::CustomMediaText,
    NodeObjFlags::RestrictCfg,
    NodeObjFlags::CustomMediaUploadButton,
    NodeObjFlags::CustomMediaRefreshButton,
    NodeObjFlags::CustomMediaBase64,
    NodeObjFlags::CustomMediaFromSourceIn,
    NodeObjFlags::CustomMediaFromSourceOut,
    NodeObjFlags::CustomMediaFromOutput,
};

enum class PinInstFlags : uint64_t
{
    None                      = 0,
    Hidden                    = 1ULL << 0, 
    RerouteSplittingPin       = 1ULL << 1, 
    Multiline                 = 1ULL << 2, 
    MultilineSet              = 1ULL << 3, 
    ColorHSV                  = 1ULL << 4, 
    ColorHex                  = 1ULL << 5, 
    ColorRaw                  = 1ULL << 6, 
    ColorFloat                = 1ULL << 7, 
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
    None                      = 0,
    ColorAsString             = 1ULL << 0,
    Input                     = 1ULL << 1,   // default for inputs
    Output                    = 1ULL << 2,   // default for outputs
    Required                  = 1ULL << 3,
    Optional                  = 1ULL << 4,
    CanLink                   = 1ULL << 5,   // default
    CanUnlink                 = 1ULL << 6,   // default
    CanRename                 = 1ULL << 7,   // default
    Advanced                  = 1ULL << 8,  
    InputCanBeWidget          = 1ULL << 9,   // default for inputs
    ImageUpload               = 1ULL << 10, 
    AudioUpload               = 1ULL << 11, 
    VideoUpload               = 1ULL << 12, 
    FileUpload                = 1ULL << 13, 
    TypeAny                   = 1ULL << 14, 
    Multiline                 = 1ULL << 15, 
    ClrAlphaEnabled           = 1ULL << 16,  // default for inputs
    CagRequire                = 1ULL << 17, 
    DenyMultilineSwitch       = 1ULL << 18, 
    OutputIsList              = 1ULL << 19, 
    ForceInput                = 1ULL << 20, 
    DefInput                  = 1ULL << 21, // TODO ??
    Lazy                      = 1ULL << 22, // TODO ??
    Multiselect               = 1ULL << 23, 
    DisplAsNumber             = 1ULL << 24, 
    DisplAsColor              = 1ULL << 25, 
    DisplAsSlider             = 1ULL << 26, 
    DynamicPrompts            = 1ULL << 27, 
    Base64Inside              = 1ULL << 28, 
    NoHistoryOnValChange      = 1ULL << 29, 
};

inline constexpr ObjectFlags<PinObjFlags> g_pin_obj_serializable_flags
{
    PinObjFlags::CagRequire,
};
