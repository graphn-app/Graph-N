/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once

class number final
{
public:
    number() noexcept = default;
    number(number_t d) noexcept
        : n(d)
    {
        static_assert(std::same_as<number_t, double>, "Check this class if number_t changes");
    }

    auto operator<=>(const number&) const noexcept = default;

    number& operator++() noexcept { ++n; return *this; }
    number operator++(int32_t) noexcept { number temp = *this; ++n; return temp; }

    number& operator--() noexcept { --n; return *this; }
    number operator--(int32_t) noexcept { number temp = *this; --n; return temp; }

    number& operator+(const number& other) noexcept { n += other.n; return *this; }
    number& operator-(const number& other) noexcept { n -= other.n; return *this; }

    number_t underlying_type() const noexcept { return n; }
    number_t* underlying_type_ref() noexcept { return &n; }
    const number_t* underlying_type_ref() const noexcept { return &n; }

private:
    number_t n = NUMBER_DEFAULT;
};

using primitive_t = std::variant<std::monostate, int64_t, double, number, std::string, bool>;

class Primitive final
{
public:
    Primitive() noexcept = default;
    ~Primitive() = default;

    template <typename T, typename = std::enable_if_t<std::is_constructible_v<primitive_t, T>>>
    Primitive(T&& val) noexcept(std::is_nothrow_constructible_v<primitive_t, T>) 
        : m_value(std::forward<T>(val)) 
    {}

    template <typename T, typename = std::enable_if_t<std::is_constructible_v<primitive_t, T>>>
    Primitive& operator=(T&& val) noexcept(std::is_nothrow_assignable_v<primitive_t, T>)
    {
        m_value = std::forward<T>(val);
        return *this;
    }

    auto operator<=>(const Primitive&) const = default;

    [[nodiscard]] bool is_null() const noexcept { return std::holds_alternative<std::monostate>(m_value); }
    [[nodiscard]] bool is_int() const noexcept { return std::holds_alternative<int64_t>(m_value); }
    [[nodiscard]] bool is_float() const noexcept { return std::holds_alternative<double>(m_value); }
    [[nodiscard]] bool is_number() const noexcept { return std::holds_alternative<number>(m_value); }
    [[nodiscard]] bool is_bool() const noexcept { return std::holds_alternative<bool>(m_value); }
    [[nodiscard]] bool is_string() const noexcept { return std::holds_alternative<std::string>(m_value); }

    [[nodiscard]] int64_t* get_int_ref() noexcept { return std::get_if<int64_t>(&m_value); }
    [[nodiscard]] double* get_float_ref() noexcept { return std::get_if<double>(&m_value); }
    [[nodiscard]] number* get_number_ref() noexcept { return std::get_if<number>(&m_value); }
    [[nodiscard]] bool* get_bool_ref() noexcept { return std::get_if<bool>(&m_value); }
    [[nodiscard]] std::string* get_string_ref() noexcept { return std::get_if<std::string>(&m_value); }

    [[nodiscard]] const int64_t* get_int_ref() const noexcept { return std::get_if<int64_t>(&m_value); }
    [[nodiscard]] const double* get_float_ref() const noexcept { return std::get_if<double>(&m_value); }
    [[nodiscard]] const number* get_number_ref() const noexcept { return std::get_if<number>(&m_value); }
    [[nodiscard]] const bool* get_bool_ref() const noexcept { return std::get_if<bool>(&m_value); }
    [[nodiscard]] const std::string* get_string_ref() const noexcept { return std::get_if<std::string>(&m_value); }

    [[nodiscard]] int64_t get_int() const noexcept { const int64_t* ref = get_int_ref(); return (nullptr == ref ? 0 : *ref); }
    [[nodiscard]] double get_float() const noexcept { const double* ref = get_float_ref(); return (nullptr == ref ? 0.0 : *ref); }
    [[nodiscard]] number get_number() const noexcept { const number* ref = get_number_ref(); return (nullptr == ref ? number{} : *ref); }
    [[nodiscard]] bool get_bool() const noexcept { const bool* ref = get_bool_ref(); return (nullptr == ref ? false : *ref); }
    [[nodiscard]] const std::string& get_string() const noexcept
    {
        const std::string* ref = get_string_ref();
        if (ref == nullptr)
        {
            static std::string empty;
            return empty;
        }
        return *ref;
    }

    [[nodiscard]] primitive_t& value() noexcept { return m_value; }
    [[nodiscard]] const primitive_t& value() const noexcept { return m_value; }

    void reset() noexcept { m_value = std::monostate(); }

    template <typename T>
    bool clamp(T min_val, T max_val) noexcept
    {
        T* current = std::get_if<T>(&m_value);
        if (!current) 
            return false;

        const T clamped = std::clamp(*current, min_val, max_val);
        if (*current != clamped)
        {
            *current = clamped;
            return true;
        }

        return false;
    }

private:
    primitive_t m_value = std::monostate();
};
