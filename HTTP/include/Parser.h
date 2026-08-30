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
    enum class ParseResult
    {
        Error,
        Incomplete,
        Complete,
        OK
    };

    enum class ParseState
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

    struct SVKV
    {
        std::string_view Key = "";
        std::string_view Value = "";
    };

    struct Request
    {
        std::vector<SVKV> Headers{};
        std::vector<SVKV> Trailers{};
        std::vector<std::string_view> ChunkedBody{};
        std::string_view Method = "";
        std::string_view URI = "";
        std::string_view Path = "";
        std::string_view Query = "";
        std::string_view Version = "";
        std::string_view Body = "";
        size_t ContentLength = 0;
    };

    struct Chunk
    {
        std::string_view Data = "";
        size_t Size = 0;
    };
	
	class Parser
    {
    public:
        Request Req{};
        std::string Buffer = "";
        SVKV CurrentHeader{};
        SVKV CurrentTrailer{};
        Chunk CurrentChunk{};
        size_t Position = 0;
        ParseState State = ParseState::RequestLineStart;

        Parser() = default;
        ~Parser() = default;

        int Parse();
        void ParseURIComponents();
        void Reset();
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
