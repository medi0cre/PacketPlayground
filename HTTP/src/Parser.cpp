#include <Parser.h>
#include <Utils.h>
#include <Server.h>
#include <Logger.h>
#include <charconv>

int PPG::Parser::Parse()
{
    while (true)
    {
        ParseResult Result = ParseByState();
        Logger::Get().Trace("State: " + std::to_string(static_cast<int>(State)) + ", ParseResult: " + std::to_string(static_cast<int>(Result)));

        switch (Result)
        {
        case ParseResult::Incomplete: { return 0; }
        case ParseResult::Error: { return -1; }
        case ParseResult::Complete: { return 1; }
        default:
        {
            Enforce(Result == ParseResult::OK, "Invariant broken inside Parse() function");
            break;
        }
        }
    }
}

PPG::ParseResult PPG::Parser::ParseByState()
{
    switch (State)
    {
    case ParseState::RequestLineStart: { return ParseRequestLineStart(); }
    case ParseState::RequestLineMethod: { return ParseRequestLineMethod(); }
    case ParseState::RequestLineURI: { return ParseRequestLineURI(); }
    case ParseState::RequestLineVersion: { return ParseRequestLineVersion(); }
    case ParseState::RequestLineCRLF: { return ParseRequestLineCRLF(); }
    case ParseState::HeaderName: { return ParseHeaderName(); }
    case ParseState::HeaderValueStart: { return ParseHeaderValueStart(); }
    case ParseState::HeaderValue: { return ParseHeaderValue(); }
    case ParseState::HeaderValueCRLF: { return ParseHeaderValueCRLF(); }
    case ParseState::HeaderEndCRLF: { return ParseHeaderEndCRLF(); }
    case ParseState::BodyFixedLength: { return ParseBodyFixedLength(); }
    case ParseState::BodyChunkSize: { return ParseChunkSize(); }
    case ParseState::BodyChunkExtension: { return ParseChunkExtension(); }
    case ParseState::BodyChunkSizeCRLF: { return ParseChunkSizeCRLF(); }
    case ParseState::BodyChunkData: { return ParseChunkData(); }
    case ParseState::BodyChunkDataCRLF: { return ParseChunkDataCRLF(); }
    case ParseState::BodyChunkTrailerName: { return ParseChunkTrailerName(); }
    case ParseState::BodyChunkTrailerValue: { return ParseChunkTrailerValue(); }
    case ParseState::BodyChunkTrailerCRLF: { return ParseChunkTrailerCRLF(); }
    case ParseState::BodyChunkFinalCRLF: { return ParseChunkFinalCRLF(); }
    default: { return ParseResult::Error; }
    }
}

PPG::ParseResult PPG::Parser::ParseRequestLineStart()
{
    while (Position < Buffer.size())
    {
        // NGINX style: Blank lines before method are okay but spaces and tabs are not
        char Byte = Buffer[Position];
        if (Byte == '\r' || Byte == '\n')
        {
            Position++;
            continue;
        }
        else if (Byte == ' ' || Byte == '\t') { return ParseResult::Error; }
        else { break; }
    }

    if (Position >= Buffer.size()) { return ParseResult::Incomplete; }

    State = ParseState::RequestLineMethod;
    return ParseResult::OK;
}

PPG::ParseResult PPG::Parser::ParseRequestLineMethod()
{
    size_t MethodStart = Position;
    while (Position < Buffer.size())
    {
        if (Position - MethodStart > MaxMethodSize) { return ParseResult::Error; }
        char Byte = Buffer[Position];
        if (Byte == ' ')
        {
            Req.Method = std::string_view(Buffer.data() + MethodStart, Position - MethodStart);
            if (!IsValidMethod(Req.Method)) { return ParseResult::Error; }

            Position++;
            State = ParseState::RequestLineURI;
            return ParseResult::OK;
        }
        else if (Byte == '\r' || Byte == '\n' || Byte == '\t') { return ParseResult::Error; }
        else { Position++; }
    }

    return ParseResult::Incomplete;
}

PPG::ParseResult PPG::Parser::ParseRequestLineURI()
{
    // NGINX style: Accept multiple spaces but not tabs or newlines before URI
    while (Position < Buffer.size() && Buffer[Position] == ' ') { Position++; }

    size_t URIStart = Position;
    while (Position < Buffer.size())
    {
        if (Position - URIStart > MaxURISize) { return ParseResult::Error; }
        char Byte = Buffer[Position];
        if (Byte == ' ')
        {
            Req.URI = std::string_view(Buffer.data() + URIStart, Position - URIStart);
            ParseURIComponents();

            Position++;
            State = ParseState::RequestLineVersion;
            return ParseResult::OK;
        }
        else if (Byte == '\r' || Byte == '\n' || Byte == '\t') { return ParseResult::Error; }
        else { Position++; }
    }

    return ParseResult::Incomplete;
}

PPG::ParseResult PPG::Parser::ParseRequestLineVersion()
{
    // NGINX style: Accept multiple spaces but not tabs or newlines before version
    while (Position < Buffer.size() && Buffer[Position] == ' ') { Position++; }

    size_t VersionStart = Position;
    while (Position < Buffer.size())
    {
        if (Position - VersionStart > MaxVersionSize) { return ParseResult::Error; }
        char Byte = Buffer[Position];
        if (Byte == '\r')
        {
            Req.Version = std::string_view(Buffer.data() + VersionStart, Position - VersionStart);
            if (!IsValidHTTPVersion(Req.Version)) { return ParseResult::Error; }

            State = ParseState::RequestLineCRLF;
            return ParseResult::OK;
        }
        else if (Byte == '\n' || Byte == '\t' || Byte == ' ') { return ParseResult::Error; }
        else { Position++; }
    }

    return ParseResult::Incomplete;
}

PPG::ParseResult PPG::Parser::ParseRequestLineCRLF()
{
    if (Position + 1 < Buffer.size())
    {
        if (Buffer[Position] == '\r' && Buffer[Position + 1] == '\n')
        {
            Position += 2;
            State = ParseState::HeaderName;
            return ParseResult::OK;
        }
        else { return ParseResult::Error; }
    }

    return ParseResult::Incomplete;
}

PPG::ParseResult PPG::Parser::ParseHeaderName()
{
    if (Position + 1 < Buffer.size())
    {
        if (Buffer[Position] == '\r' && Buffer[Position + 1] == '\n')
        {
            State = ParseState::HeaderEndCRLF;
            return ParseResult::OK;
        }
    }

    size_t NameStart = Position;
    while (Position < Buffer.size())
    {
        if (Position - NameStart > MaxHeaderNameSize) { return ParseResult::Error; }
        char Byte = Buffer[Position];
        if (Byte == ':')
        {
            CurrentHeader.Key = std::string_view(Buffer.data() + NameStart, Position - NameStart);
            if (!IsValidToken(CurrentHeader.Key)) { return ParseResult::Error; }

            Position++;
            State = ParseState::HeaderValueStart;
            return ParseResult::OK;

        }
        else if (Byte == ' ' || Byte == '\t') { return ParseResult::Error; }
        else { Position++; }
    }

    return ParseResult::Incomplete;
}

PPG::ParseResult PPG::Parser::ParseHeaderValueStart()
{
    while (Position < Buffer.size())
    {
        char Byte = Buffer[Position];
        if (Byte == ' ' || Byte == '\t') { Position++; }
        else { break; }
    }

    if (Position >= Buffer.size()) { return ParseResult::Incomplete; }
    State = ParseState::HeaderValue;
    return ParseResult::OK;
}

PPG::ParseResult PPG::Parser::ParseHeaderValue()
{
    size_t ValueStart = Position;

    while(Position < Buffer.size())
    {
        if (Position - ValueStart > MaxHeaderValueSize) { return ParseResult::Error; }
        char Byte = Buffer[Position];
        if (Byte == '\r')
        {
            CurrentHeader.Value = std::string_view(Buffer.data() + ValueStart, Position - ValueStart);
            TrimTrailingWhiteSpace(CurrentHeader.Value);

            State = ParseState::HeaderValueCRLF;
            return ParseResult::OK;
        }
        else if (Byte == '\0' || Byte == '\n') { return ParseResult::Error; }
        else { Position++; }
    }

    return ParseResult::Incomplete;
}

PPG::ParseResult PPG::Parser::ParseHeaderValueCRLF()
{
    if (Position + 1 < Buffer.size())
    {
        if (Buffer[Position] == '\r' && Buffer[Position + 1] == '\n')
        {
            Position += 2;
            Req.Headers.emplace_back(CurrentHeader);
            if (Req.Headers.size() > MaxHeaderCount) { return ParseResult::Error; }

            CurrentHeader = {};
            State = ParseState::HeaderName;
            return ParseResult::OK;
        }
        else { return ParseResult::Error; }
    }

    return ParseResult::Incomplete;
}

PPG::ParseResult PPG::Parser::ParseHeaderEndCRLF()
{
    if (Position + 1 < Buffer.size())
    {
        if (Buffer[Position] == '\r' && Buffer[Position + 1] == '\n')
        {
            Position += 2;
            return FinalizeHeaders();
        }
        else { return ParseResult::Error; }
    }

    return ParseResult::Incomplete;
}

PPG::ParseResult PPG::Parser::ParseBodyFixedLength()
{
    if (Req.ContentLength > MaxBodySize) { return ParseResult::Error; }

    if (Req.ContentLength == 0)
    {
        State = ParseState::ParseComplete;
        return ParseResult::Complete;
    }

    if (Buffer.size() - Position < Req.ContentLength) { return ParseResult::Incomplete; }

    Req.Body = std::string_view(Buffer.data() + Position, Req.ContentLength);
    Position += Req.ContentLength;

    State = ParseState::ParseComplete;
    return ParseResult::Complete;
}

PPG::ParseResult PPG::Parser::ParseChunkSize()
{
    size_t SizeStart = Position;

    while (Position < Buffer.size())
    {
        char Byte = Buffer[Position];
        if (IsHexDigit(Byte)) { Position++; }
        else if (Byte == '\r')
        {
            long long HexSize = ParseHex(std::string_view(Buffer.data() + SizeStart, Position - SizeStart));

            if (HexSize < 0 || static_cast<size_t>(HexSize) > MaxChunkSize) { return ParseResult::Error; }
            else { CurrentChunk.Size = static_cast<size_t>(HexSize); }

            State = ParseState::BodyChunkSizeCRLF;
            return ParseResult::OK;
        }
        else if (Byte == ';')
        {
            long long HexSize = ParseHex(std::string_view(Buffer.data() + SizeStart, Position - SizeStart));

            if (HexSize < 0 || static_cast<size_t>(HexSize) > MaxChunkSize) { return ParseResult::Error; }
            else { CurrentChunk.Size = static_cast<size_t>(HexSize); }

            Position++;
            State = ParseState::BodyChunkExtension;
            return ParseResult::OK;
        }
        else { return ParseResult::Error; }
    }

    return ParseResult::Incomplete;
}

PPG::ParseResult PPG::Parser::ParseChunkExtension()
{
    // We don't give a flying fuck about chunk extensions around here!
    size_t ExtensionStart = Position;
    while (Position < Buffer.size())
    {
        if (Position - ExtensionStart > MaxChunkExtensionSize) { return ParseResult::Error; }
        char Byte = Buffer[Position];
        if (Byte == '\r')
        {
            State = ParseState::BodyChunkSizeCRLF;
            return ParseResult::OK;
        }
        else { Position++; }
    }

    return ParseResult::Incomplete;
}

PPG::ParseResult PPG::Parser::ParseChunkSizeCRLF()
{
    if (Position + 1 < Buffer.size())
    {
        if (Buffer[Position] == '\r' && Buffer[Position + 1] == '\n')
        {
            Position += 2;
            if (CurrentChunk.Size == 0) { State = ParseState::BodyChunkTrailerName; }
            else
            {
                CurrentChunk = {};
                State = ParseState::BodyChunkData;
            }

            return ParseResult::OK;
        }
        else { return ParseResult::Error; }
    }

    return ParseResult::Incomplete;
}

PPG::ParseResult PPG::Parser::ParseChunkData()
{
    // No need for boundary checks since both size == 0 and limit exceeded were checked in previous states
    if (Buffer.size() - Position < CurrentChunk.Size) { return ParseResult::Incomplete; }

    Req.ChunkedBody.emplace_back(std::string_view(Buffer.data() + Position, CurrentChunk.Size));
    Position += CurrentChunk.Size;

    State = ParseState::BodyChunkDataCRLF;
    return ParseResult::OK;
}

PPG::ParseResult PPG::Parser::ParseChunkDataCRLF()
{
    if (Position + 1 < Buffer.size())
    {
        if (Buffer[Position] == '\r' && Buffer[Position + 1] == '\n')
        {
            Position += 2;
            CurrentChunk = {};
            State = ParseState::BodyChunkSize;
            return ParseResult::OK;
        }
        else { return ParseResult::Error; }
    }

    return ParseResult::Incomplete;
}

PPG::ParseResult PPG::Parser::ParseChunkTrailerName()
{
    if (Position + 1 < Buffer.size())
    {
        if (Buffer[Position] == '\r' && Buffer[Position + 1] == '\n')
        {
            State = ParseState::BodyChunkFinalCRLF;
            return ParseResult::OK;
        }
    }

    size_t NameStart = Position;
    while (Position < Buffer.size())
    {
        if  (Position - NameStart > MaxTrailerNameSize) { return ParseResult::Error; }
        char Byte = Buffer[Position];
        if (Byte == ':')
        {
            CurrentTrailer.Key = std::string_view(Buffer.data() + NameStart, Position - NameStart);
            if (!IsValidToken(CurrentTrailer.Key)) { return ParseResult::Error; }

            Position++;
            State = ParseState::BodyChunkTrailerValue;
            return ParseResult::OK;
        }
        else if (Byte == ' ' || Byte == '\t') { return ParseResult::Error; }
        else { Position++; }
    }

    return ParseResult::Incomplete;
}

PPG::ParseResult PPG::Parser::ParseChunkTrailerValue()
{
    while (Position < Buffer.size())
    {
        char Byte = Buffer[Position];
        if (Byte == ' ' || Byte == '\t') { Position++; }
        else { break; }
    }

    if (Position >= Buffer.size()) { return ParseResult::Incomplete; }

    size_t ValueStart = Position;
    while(Position < Buffer.size())
    {
        if (Position - ValueStart > MaxTrailerValueSize) { return ParseResult::Error; }
        char Byte = Buffer[Position];
        if (Byte == '\r')
        {
            CurrentTrailer.Value = std::string_view(Buffer.data() + ValueStart, Position - ValueStart);
            TrimTrailingWhiteSpace(CurrentTrailer.Value);

            State = ParseState::BodyChunkTrailerCRLF;
            return ParseResult::OK;
        }
        else if (Byte == '\0' or Byte == '\n') { return ParseResult::Error; }
        else { Position++; }
    }

    return ParseResult::Incomplete;
}

PPG::ParseResult PPG::Parser::ParseChunkTrailerCRLF()
{
    if (Position + 1 < Buffer.size())
    {
        if (Buffer[Position] == '\r' && Buffer[Position + 1] == '\n')
        {
            Position += 2;
            Req.Trailers.emplace_back(CurrentTrailer);
            if (Req.Trailers.size() > MaxTrailerCount) { return ParseResult::Error; }

            CurrentTrailer = {};
            State = ParseState::BodyChunkTrailerName;
            return ParseResult::OK;
        }
        else { return ParseResult::Error; }
    }

    return ParseResult::Incomplete;
}

PPG::ParseResult PPG::Parser::ParseChunkFinalCRLF()
{
    if (Position + 1 < Buffer.size())
    {
        if (Buffer[Position] == '\r' && Buffer[Position + 1] == '\n')
        {
            Position += 2;
            State = ParseState::ParseComplete;
            return ParseResult::Complete;
        }
        else { return ParseResult::Error; }
    }

    return ParseResult::Incomplete;
}

void PPG::Parser::ParseURIComponents()
{
    size_t QuestionMarkPosition = Req.URI.find('?');
    if (QuestionMarkPosition != std::string::npos)
    {
        Logger::Get().Trace("URI contains a query");
        Req.Path = Req.URI.substr(0, QuestionMarkPosition);
        Req.Query = Req.URI.substr(QuestionMarkPosition + 1);
    }
    else
    {
        Logger::Get().Trace("URI does not contain a query");
        Req.Path = Req.URI;
        Req.Query = "";
    }

    // Req.Path = URLDecode(Req.Path);
    // Logger::Get().Trace("Decoded URL: " + Req.Path);
}

PPG::ParseResult PPG::Parser::FinalizeHeaders()
{
    bool CLFlag = false;
    bool TEFlag = false;
    bool HostFlag = false;
    size_t CLIndex = 0;
    size_t TEIndex = 0;
    size_t HostIndex = 0;

    for (size_t i = 0; i < Req.Headers.size(); i++)
    {
        if (CompareInsensitive(Req.Headers[i].Key, "host"))
        {
            // Reject duplicate host headers
            if (!HostFlag)
            {
                HostFlag = true;
                HostIndex = i;
            }
            else { return ParseResult::Error; }
        }

        if (CompareInsensitive(Req.Headers[i].Key, "transfer-encoding"))
        {
            // Reject duplicate transfer encoding headers
            if (!TEFlag)
            {
                TEFlag = true;
                TEIndex = i;
            }
            else { return ParseResult::Error; }
        }

        if (CompareInsensitive(Req.Headers[i].Key, "content-length"))
        {
            // Reject duplicate content length headers
            if (!CLFlag)
            {
                CLFlag = true;
                CLIndex = i;
            }
            else { return ParseResult::Error; }
        }
    }

    if ((Req.Version == "HTTP/1.1" && !HostFlag)
        || (HostFlag && Req.Headers[HostIndex].Value == "")) { return ParseResult::Error; }

    if (CLFlag && TEFlag) { return ParseResult::Error; }
    else if (TEFlag)
    {
        // For now we only deal with chunked encoding for the HTTP 1.1 server
        // Whether we upgrade later to include gzip is still undecided
        std::string_view TransferEncoding = Req.Headers[TEIndex].Value;
        if (CompareInsensitive(TransferEncoding, "chunked"))
        {
            State = ParseState::BodyChunkSize;
            return ParseResult::OK;
        }
        else { return ParseResult::Error; }
    }
    else if (CLFlag)
    {
		long long Length = 0;
		auto [Ptr, Err] = std::from_chars(Req.Headers[CLIndex].Value.data(),
				Req.Headers[CLIndex].Value.data() + Req.Headers[CLIndex].Value.size(),
				Length);

		if (Err != std::errc{} || Ptr != (Req.Headers[CLIndex].Value.data() + Req.Headers[CLIndex].Value.size())) { return ParseResult::Error; }
		if (Length < 0) { return ParseResult::Error; }

		Req.ContentLength = static_cast<size_t>(Length);
		if (Req.ContentLength == 0)
		{
			State = ParseState::ParseComplete;
			return ParseResult::Complete;
		}
		else
		{
			State = ParseState::BodyFixedLength;
			return ParseResult::OK;
		}
    }

    State = ParseState::ParseComplete;
    return ParseResult::Complete;
}

void PPG::Parser::Reset()
{
    Req = {};
    CurrentHeader = {};
    CurrentTrailer = {};
    CurrentChunk = {};
    State = ParseState::RequestLineStart;
}
