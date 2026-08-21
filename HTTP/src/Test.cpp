#include <iostream>
#include <Test.h>
#include <Utils.h>
#include <Parser.h>

void RunParserTests()
{
    PPG::Parser P{};
    std::string str1 = "GET /index.html HTTP/1.1\r\n"
                       "header1: val";

    std::string str2 =             "ue1\r\n"
                       "header2: value2\r\n"
                       "header3: va";

    std::string str3 =            "lue3\r\n"
                       "\r\n"
                       "lorem ipsum ewjebwk";
    
    std::cerr << "lorem ipsum nksajnksa";
}

void RunTests()
{
    RunParserTests();
}
