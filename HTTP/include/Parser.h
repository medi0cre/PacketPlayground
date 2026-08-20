#pragma once
#include <string>
#include <vector>

namespace HTTP
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
        std::string Key = "";
        std::string Value = "";
    };

    struct Request
    {
        std::string Method = "";
        std::string URI = "";
        std::string Path = "";
        std::string Query = "";
        std::string Version = "";
        std::vector<KV> Headers{};
        std::vector<KV> Trailers{};
        std::string Body = "";
        ParseState State = ParseState::RequestLineStart;
        size_t ContentLength = 0;
        bool IsChunked = false;
        bool IsComplete = false;
    };

    struct Chunk
    {
        size_t Size = 0;
        size_t BytesRead = 0;
        bool IsFinal = false;
        std::string Data = "";
    };
	
	class Parser
    {
    public:
        size_t Position = 0;
        size_t BytesRemaining = 0;
        Request Req{};
        ParseState State = ParseState::RequestLineStart;
        std::string Buffer = "";
        std::string CurrentHeaderName = "";
        std::string CurrentHeaderValue = "";
        std::string CurrentTrailerName = "";
        std::string CurrentTrailerValue = "";
        std::string ChunkedBody = "";
        Chunk CurrentChunk{};

        Parser();
        void Reset();
        void ParseURIComponents();
        int Parse(std::string Data, size_t DataLength);

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
