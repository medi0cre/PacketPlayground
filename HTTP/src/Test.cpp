#include <iostream>
#include <Test.h>
#include <Utils.h>
#include <Parser.h>

void RunParserTests()
{
    HTTP::Parser P{};
    std::string str1 = "GET /index.html HTTP/1.1\r\n"
                       "header1: val";

    std::string str2 =             "ue1\r\n"
                       "header2: value2\r\n"
                       "header3: va";

    std::string str3 =            "lue3\r\n"
                       "\r\n"
                       "lorem ipsum ewjebwk";

    int res1 = P.Parse(str1, str1.size());
    int res2 = P.Parse(str2, str2.size());
    int res3 = P.Parse(str3, str3.size());
    std::cout << res1 << "\n";
    std::cout << res2 << "\n";
    std::cout << res3 << "\n";

    Enforce(false, "few");
}

void RunTests()
{
    RunParserTests();
}
