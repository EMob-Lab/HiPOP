#include "helpers.h"

#include <hipop/string_util.h>

#include <string>
#include <string_view>


void test_StrAppend() {

    // General case, with the different types of string-like arguments.
    {
        std::string text = "Hello";
        const char *ptr = " ";
        std::string_view sv = "!";

        hipop::StrAppend(text, ptr, "World", sv);
        assertTrue(text == "Hello World!", "[StrAppend] General case not working");
    }

    // Edge cases with 0 or 1 argument.
    {
        std::string result = "Nothing else";
        hipop::StrAppend(result);
        assertTrue(result == "Nothing else", "[StrAppend] Edge case with 0 arguments not working");
    }
    {
        std::string result = "Only";
        hipop::StrAppend(result, "-one");
        assertTrue(result == "Only-one", "[StrAppend] Edge case with 1 argument not working");
    }
}


void test_StrCat() {

    // General case, with the different types of string-like arguments.
    {
        std::string_view sv = "Hello";
        const char *ptr = " ";
        std::string str = "World";

        std::string result = hipop::StrCat(sv, ptr, str, "!");
        assertTrue(result == "Hello World!", "[StrCat] General case not working");
    }

    // Edge cases with 0 or 1 argument.
    {
        std::string result = hipop::StrCat();
        assertTrue(result == "", "[StrCat] Edge case with 0 arguments not working");
    }
    {
        std::string result = hipop::StrCat("Only-one");
        assertTrue(result == "Only-one", "[StrCat] Edge case with 1 argument not working");
    }
}


int testStringUtil(int, char**) {
    test_StrAppend();
    test_StrCat();
    return 0;
}
