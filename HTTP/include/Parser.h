#pragma once
#include <string>
#include <string_view>
#include <vector>

constexpr size_t MaxChunkExtensionSize = 1024;
constexpr size_t MaxMethodSize = 16;
constexpr size_t MaxHeaderNameSize = 256;
constexpr size_t MaxTrailerNameSize = 256;
constexpr size_t MaxHeaderValueSize = 256;
constexpr size_t MaxTrailerValueSize = 256;
constexpr size_t MaxURISize = 256;
constexpr size_t MaxVersionSize = 8;
constexpr size_t MaxHeaderCount = 128;
constexpr size_t MaxTrailerCount = 32;
constexpr size_t MaxChunkSize = 4096;
constexpr size_t MaxBodySize = 30720;

namespace PPG
{
    enum ParseResult
    {
        Error,
        Incomplete,
        Complete,
        OK
    };

    enum ParseState
    {
        RequestLineStart,
        RequestLineMethod,
        RequestLineURI,
        RequestLineVersion,
        RequestLineCRLF,
        HeaderName,
        HeaderValueStart,
        HeaderValue,
        HeaderValueCRLF,
        HeaderEndCRLF,
        BodyFixedLength,
        BodyChunkSize,
        BodyChunkExtension,
        BodyChunkSizeCRLF,
        BodyChunkData,
        BodyChunkDataCRLF,
        BodyChunkTrailerName,
        BodyChunkTrailerValue,
        BodyChunkTrailerCRLF,
        BodyChunkFinalCRLF,
        ParseComplete,
    };

    struct KV
    {
        std::string_view Key;
        std::string_view Value;
    };

    struct Request
    {
        std::string_view Method;
        std::string_view URI;
        std::string_view Path;
        std::string_view Query;
        std::string_view Version;
        std::vector<KV> Headers;
        std::vector<KV> Trailers;
        std::string_view Body;
        size_t ContentLength = 0;
        std::vector<std::string_view> ChunkedBody;
    };

    struct Chunk
    {
        size_t Size = 0;
        std::string_view Data;
    };
	
	class Parser
    {
    public:
        size_t Position = 0;
        Request Req{};
        ParseState State = ParseState::RequestLineStart;
        std::string Buffer = "";
        KV CurrentHeader{};
        KV CurrentTrailer{};
        Chunk CurrentChunk{};

        Parser() = default;
        ~Parser() = default;

        void ParseURIComponents();
        int Parse();

        ParseResult FinalizeHeaders();

        ParseResult ParseByState();
        ParseResult ParseRequestLineStart();
        ParseResult ParseRequestLineMethod();
        ParseResult ParseRequestLineURI();
        ParseResult ParseRequestLineVersion();
        ParseResult ParseRequestLineCRLF();
        ParseResult ParseHeaderName();
        ParseResult ParseHeaderValueStart();
        ParseResult ParseHeaderValue();
        ParseResult ParseHeaderValueCRLF();
        ParseResult ParseHeaderEndCRLF();
        ParseResult ParseBodyFixedLength();
        ParseResult ParseChunkSize();
        ParseResult ParseChunkExtension();
        ParseResult ParseChunkSizeCRLF();
        ParseResult ParseChunkData();
        ParseResult ParseChunkDataCRLF();
        ParseResult ParseChunkTrailerName();
        ParseResult ParseChunkTrailerValue();
        ParseResult ParseChunkTrailerCRLF();
        ParseResult ParseChunkFinalCRLF();
    };
}
