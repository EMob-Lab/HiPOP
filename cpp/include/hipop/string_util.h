#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>


namespace hipop {


    /**
     * Append multiple arguments to the target string (which gets modified in place).
     *
     * @tparam Args Must be a litteral, a C-string (const char*), a std::string, a std::string_view,
     *              or any type convertible to std::string_view.
     */
    template<typename... Args>
    void StrAppend(std::string &target, Args&&... args) {
        static_assert(
            (std::is_convertible_v<Args, std::string_view> && ...),
            "All arguments passed to StrAppend/StrCat must be convertible to std::string_view"
        );
        // NOLINTBEGIN (cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        std::size_t total_size = (target.size() + ... + std::string_view(args).size());
        target.reserve(total_size);
        (target.append(std::forward<Args>(args)), ...);
        // NOLINTEND (cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    }


    /**
     * Concatenate multiple arguments into a single string.
     *
     * @tparam Args Must be a litteral, a C-string (const char*), a std::string, a std::string_view,
     *              or any type convertible to std::string_view.
     */
    template<typename... Args>
    std::string StrCat(Args&&... args) {
        std::string result;
        StrAppend(result, std::forward<Args>(args)...);
        return result;
    }


} // namespace hipop
