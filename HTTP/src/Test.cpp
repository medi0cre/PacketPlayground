#include <iostream>
#include <Test.h>
#include <Utils.h>
#include <Parser.h>

void RunParserTests()
{
    HTTP::Parser P{};
    P.Buffer = "\n\nGET   /index.html   HTTP/1.1\r\n"
               "header1: value1\r\n"
               "header2: value2\r\n"
               "content-length: 19\r\n"
               "\r\n"
               "lorem ipsum ewjebwk";

    Enforce(P.ParseRequestLineStart() == HTTP::ParseResult::OK, "cvwghvwh");
    Enforce(P.ParseRequestLineMethod() == HTTP::ParseResult::OK && P.Req.Method == "GET", "cvwghvwh");
    Enforce(P.ParseRequestLineURI() == HTTP::ParseResult::OK && P.Req.URI == "/index.html", "cvwghvwh");
    Enforce(P.ParseRequestLineVersion() == HTTP::ParseResult::OK && P.Req.Version == "HTTP/1.1", "cvwghvwh");
    Enforce(P.ParseRequestLineCRLF() == HTTP::ParseResult::OK, "cvwghvwh1");
    Enforce(P.ParseHeaderName() == HTTP::ParseResult::OK, "cvwghvwh1");
    Enforce(P.ParseHeaderValueStart() == HTTP::ParseResult::OK, "cvwghvwh1");
    Enforce(P.ParseHeaderValue() == HTTP::ParseResult::OK, "cvwghvwh1");
    Enforce(P.ParseHeaderValueCRLF() == HTTP::ParseResult::OK, "cvwghvwh1");
    Enforce(P.ParseHeaderName() == HTTP::ParseResult::OK, "cvwghvwh1");
    Enforce(P.ParseHeaderValueStart() == HTTP::ParseResult::OK, "cvwghvwh1");
    Enforce(P.ParseHeaderValue() == HTTP::ParseResult::OK, "cvwghvwh1");
    Enforce(P.ParseHeaderValueCRLF() == HTTP::ParseResult::OK, "cvwghvwh1");
    Enforce(P.ParseHeaderName() == HTTP::ParseResult::OK, "cvwghvwh1");
    Enforce(P.ParseHeaderValueStart() == HTTP::ParseResult::OK, "cvwghvwh1");
    Enforce(P.ParseHeaderValue() == HTTP::ParseResult::OK, "cvwghvwh1");
    Enforce(P.ParseHeaderValueCRLF() == HTTP::ParseResult::OK, "cvwghvwh1");
    Enforce(P.ParseHeaderName() == HTTP::ParseResult::OK, "cvwghvwh1");
    Enforce(P.ParseHeaderEndCRLF() == HTTP::ParseResult::OK, "Failed 1");
    Enforce(P.ParseBodyFixedLength() == HTTP::ParseResult::Complete, "Failed 2");

    for (const auto& [key, value] : P.Req.Headers) 
    {
        std::cout << key << " -> " << value << '\n';
    }

    std::cout << P.Req.Body << "\n";
    std::cout << P.Req.ContentLength << "\n";
    std::cout << P.Req.IsChunked << "\n";
    std::cout << P.Req.IsComplete << "\n";
    std::cout << P.Req.Method << "\n";
    std::cout << P.Req.Path << "\n";
    std::cout << P.Req.Query << "\n";
    std::cout << P.Req.State << "\n";
    std::cout << P.Req.URI << "\n";
    std::cout << P.Req.Version << "\n";
}

void RunTests()
{
    RunParserTests();
}
