#include <iostream>
#include <Parser.h>
#include <Utils.h>
#include <Server.h>

int HTTP::Parser::Parse(const std::string& Data)
{
    Buffer += Data;
    if (Buffer.size() > MaxBufferSize) { return -1; }

    while (true)
    {
        ParseResult Result = ParseByState();

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

HTTP::ParseResult HTTP::Parser::ParseByState()
{
    switch (State)
    {
    case RequestLineStart: { return ParseRequestLineStart(); }
    case RequestLineMethod: { return ParseRequestLineMethod(); }
    case RequestLineURI: { return ParseRequestLineURI(); }
    case RequestLineVersion: { return ParseRequestLineVersion(); }
    case RequestLineCRLF: { return ParseRequestLineCRLF(); }
    case HeaderName: { return ParseHeaderName(); }
    case HeaderValueStart: { return ParseHeaderValueStart(); }
    case HeaderValue: { return ParseHeaderValue(); }
    case HeaderValueCRLF: { return ParseHeaderValueCRLF(); }
    case HeaderEndCRLF: { return ParseHeaderEndCRLF(); }
    case BodyFixedLength: { return ParseBodyFixedLength(); }
    case BodyChunkSize: { return ParseChunkSize(); }
    case BodyChunkExtension: { return ParseChunkExtension(); }
    case BodyChunkSizeCRLF: { return ParseChunkSizeCRLF(); }
    case BodyChunkData: { return ParseChunkData(); }
    case BodyChunkDataCRLF: { return ParseChunkDataCRLF(); }
    case BodyChunkTrailerName: { return ParseChunkTrailerName(); }
    case BodyChunkTrailerValue: { return ParseChunkTrailerValue(); }
    case BodyChunkTrailerCRLF: { return ParseChunkTrailerCRLF(); }
    case BodyChunkFinalCRLF: { return ParseChunkFinalCRLF(); }
    default: { return ParseResult::Error; }
    }
}

HTTP::ParseResult HTTP::Parser::ParseRequestLineStart()
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

HTTP::ParseResult HTTP::Parser::ParseRequestLineMethod()
{
    size_t MethodStart = Position;
    while (Position < Buffer.size())
    {
        if (Position - MethodStart > MaxMethodSize) { return ParseResult::Error; }
        char Byte = Buffer[Position];
        if (Byte == ' ')
        {
            Req.Method = Buffer.substr(MethodStart, Position - MethodStart);
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

HTTP::ParseResult HTTP::Parser::ParseRequestLineURI()
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
            Req.URI = Buffer.substr(URIStart, Position - URIStart);

            if (!IsValidURI(Req.URI)) { return ParseResult::Error; }
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

HTTP::ParseResult HTTP::Parser::ParseRequestLineVersion()
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
            Req.Version = Buffer.substr(VersionStart, Position - VersionStart);
            if (!IsValidHTTPVersion(Req.Version)) { return ParseResult::Error; }

            State = ParseState::RequestLineCRLF;
            return ParseResult::OK;
        }
        else if (Byte == '\n' || Byte == '\t' || Byte == ' ') { return ParseResult::Error; }
        else { Position++; }
    }

    return ParseResult::Incomplete;
}

HTTP::ParseResult HTTP::Parser::ParseRequestLineCRLF()
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

HTTP::ParseResult HTTP::Parser::ParseHeaderName()
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
            CurrentHeaderName = ToLower(Buffer.substr(NameStart, Position - NameStart));
            if (!IsValidToken(CurrentHeaderName)) { return ParseResult::Error; }

            Position++;
            State = ParseState::HeaderValueStart;
            return ParseResult::OK;

        }
        else if (Byte == ' ' || Byte == '\t') { return ParseResult::Error; }
        else { Position++; }
    }

    return ParseResult::Incomplete;
}

HTTP::ParseResult HTTP::Parser::ParseHeaderValueStart()
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

HTTP::ParseResult HTTP::Parser::ParseHeaderValue()
{
    size_t ValueStart = Position;

    while(Position < Buffer.size())
    {
        if (Position - ValueStart > MaxHeaderValueSize) { return ParseResult::Error; }
        char Byte = Buffer[Position];
        if (Byte == '\r')
        {
            CurrentHeaderValue = TrimTrailingWhiteSpace(Buffer.substr(ValueStart, Position - ValueStart));
            State = ParseState::HeaderValueCRLF;
            return ParseResult::OK;
        }
        else if (Byte == '\0' || Byte == '\n') { return ParseResult::Error; }
        else { Position++; }
    }

    return ParseResult::Incomplete;
}

HTTP::ParseResult HTTP::Parser::ParseHeaderValueCRLF()
{
    if (Position + 1 < Buffer.size())
    {
        if (Buffer[Position] == '\r' && Buffer[Position + 1] == '\n')
        {
            Position += 2;
            Req.Headers.emplace_back(KV { CurrentHeaderName, CurrentHeaderValue });
            if (Req.Headers.size() > MaxHeaderCount) { return ParseResult::Error; }

            CurrentHeaderName = "";
            CurrentHeaderValue = "";
            State = ParseState::HeaderName;
            return ParseResult::OK;
        }
        else { return ParseResult::Error; }
    }

    return ParseResult::Incomplete;
}

HTTP::ParseResult HTTP::Parser::ParseHeaderEndCRLF()
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

HTTP::ParseResult HTTP::Parser::ParseBodyFixedLength()
{
    if (BytesRemaining <= 0)
    {
        State = ParseState::ParseComplete;
        return ParseResult::Complete;
    }

    size_t Available = Buffer.size() - Position;
    size_t ToRead = std::min(Available, BytesRemaining);

    Req.Body += Buffer.substr(Position, ToRead);
    if (Req.Body.size() > MaxBodySize) { return ParseResult::Error; }

    Position += ToRead;
    BytesRemaining -= ToRead;

    if (BytesRemaining <= 0)
    {
        State = ParseState::ParseComplete;
        return ParseResult::Complete;
    }

    return ParseResult::Incomplete;
}

HTTP::ParseResult HTTP::Parser::ParseChunkSize()
{
    size_t SizeStart = Position;

    while (Position < Buffer.size())
    {
        char Byte = Buffer[Position];
        if (IsHexDigit(Byte)) { Position++; }
        else if (Byte == '\r')
        {
            std::string HexString = Buffer.substr(SizeStart, Position - SizeStart);
            long long HexSize = ParseHex(HexString);

            if (HexSize < 0 || static_cast<size_t>(HexSize) > MaxChunkSize) { return ParseResult::Error; }
            else { CurrentChunk.Size = HexSize; }

            State = ParseState::BodyChunkSizeCRLF;
            return ParseResult::OK;
        }
        else if (Byte == ';')
        {
            std::string HexString = Buffer.substr(SizeStart, Position - SizeStart);
            long long HexSize = ParseHex(HexString);

            if (HexSize < 0 || static_cast<size_t>(HexSize) > MaxChunkSize) { return ParseResult::Error; }
            else { CurrentChunk.Size = HexSize; }

            Position++;
            State = ParseState::BodyChunkExtension;
            return ParseResult::OK;
        }
        else { return ParseResult::Error; }
    }

    return ParseResult::Incomplete;
}

HTTP::ParseResult HTTP::Parser::ParseChunkExtension()
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

HTTP::ParseResult HTTP::Parser::ParseChunkSizeCRLF()
{
    if (Position + 1 < Buffer.size())
    {
        if (Buffer[Position] == '\r' && Buffer[Position + 1] == '\n')
        {
            Position += 2;
            if (CurrentChunk.Size == 0)
            {
                CurrentChunk.IsFinal = true;
                State = ParseState::BodyChunkTrailerName;
            }
            else
            {
                CurrentChunk.BytesRead = 0;
                CurrentChunk.Data = "";
                State = ParseState::BodyChunkData;
            }

            return ParseResult::OK;
        }
        else { return ParseResult::Error; }
    }

    return ParseResult::Incomplete;
}

HTTP::ParseResult HTTP::Parser::ParseChunkData()
{
    size_t Available = Buffer.size() - Position;
    size_t Needed = CurrentChunk.Size - CurrentChunk.BytesRead;
    size_t ToRead = std::min(Available, Needed);

    CurrentChunk.Data += Buffer.substr(Position, ToRead);
    ChunkedBody += Buffer.substr(Position, ToRead);
    if (ChunkedBody.size() > MaxBodySize) { return ParseResult::Error; }

    Position += ToRead;
    CurrentChunk.BytesRead += ToRead;

    if (CurrentChunk.BytesRead >= CurrentChunk.Size)
    {
        State = ParseState::BodyChunkDataCRLF;
        return ParseResult::OK;
    }

    return ParseResult::Incomplete;
}

HTTP::ParseResult HTTP::Parser::ParseChunkDataCRLF()
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

HTTP::ParseResult HTTP::Parser::ParseChunkTrailerName()
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
            CurrentTrailerName = ToLower(Buffer.substr(NameStart, Position - NameStart));
            if (!IsValidToken(CurrentTrailerName)) { return ParseResult::Error; }

            Position++;
            State = ParseState::BodyChunkTrailerValue;
            return ParseResult::OK;
        }
        else if (Byte == ' ' || Byte == '\t') { return ParseResult::Error; }
        else { Position++; }
    }

    return ParseResult::Incomplete;
}

HTTP::ParseResult HTTP::Parser::ParseChunkTrailerValue()
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
            CurrentTrailerValue = TrimTrailingWhiteSpace(Buffer.substr(ValueStart, Position - ValueStart));
            State = ParseState::BodyChunkTrailerCRLF;
            return ParseResult::OK;
        }
        else if (Byte == '\0' or Byte == '\n') { return ParseResult::Error; }
        else { Position++; }
    }

    return ParseResult::Incomplete;
}

HTTP::ParseResult HTTP::Parser::ParseChunkTrailerCRLF()
{
    if (Position + 1 < Buffer.size())
    {
        if (Buffer[Position] == '\r' && Buffer[Position + 1] == '\n')
        {
            Position += 2;
            Req.Trailers.emplace_back(KV { CurrentTrailerName, CurrentTrailerValue });
            if (Req.Trailers.size() > MaxTrailerCount) { return ParseResult::Error; }

            CurrentTrailerName = "";
            CurrentTrailerValue = "";
            State = ParseState::BodyChunkTrailerName;
            return ParseResult::OK;
        }
        else { return ParseResult::Error; }
    }

    return ParseResult::Incomplete;
}

HTTP::ParseResult HTTP::Parser::ParseChunkFinalCRLF()
{
    if (Position + 1 < Buffer.size())
    {
        if (Buffer[Position] == '\r' && Buffer[Position + 1] == '\n')
        {
            Position += 2;
            Req.Body = ChunkedBody;
            State = ParseState::ParseComplete;
            return ParseResult::Complete;
        }
        else { return ParseResult::Error; }
    }

    return ParseResult::Incomplete;
}

void HTTP::Parser::ParseURIComponents()
{
    size_t QuestionMarkPosition = Req.URI.find('?');
    if (QuestionMarkPosition != std::string::npos)
    {
        Req.Path = Req.URI.substr(0, QuestionMarkPosition);
        Req.Query = Req.URI.substr(QuestionMarkPosition + 1);
    }
    else
    {
        Req.Path = Req.URI;
        Req.Query = "";
    }

    Req.Path = URLDecode(Req.Path);
}

HTTP::ParseResult HTTP::Parser::FinalizeHeaders()
{
    bool CLFlag = false;
    bool TEFlag = false;
    size_t CLIndex = 0;
    size_t TEIndex = 0;

    for (size_t i = 0; i < Req.Headers.size(); i++)
    {
        if (Req.Headers[i].Key == "transfer-encoding")
        {
            // Reject duplicate transfer encoding headers
            if (!TEFlag)
            {
                TEFlag = true;
                TEIndex = i;
            }
            else { return ParseResult::Error; }
        }

        if (Req.Headers[i].Key == "content-length")
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

    if (CLFlag && TEFlag) { return ParseResult::Error; }
    else if (TEFlag)
    {
        // For now we only deal with chunked encoding for the HTTP 1.1 server
        // Whether we upgrade later to include gzip is still undecided
        std::string TransferEncoding = ToLower(Req.Headers[TEIndex].Value);
        if (TransferEncoding == "chunked")
        {
            Req.IsChunked = true;
            State = ParseState::BodyChunkSize;
            return ParseResult::OK;
        }
        else { return ParseResult::Error; }
    }
    else if (CLFlag)
    {
        try
        {
            long long Length = std::stoll(Req.Headers[CLIndex].Value);
            if (Length < 0) { return ParseResult::Error; }
            Req.ContentLength = Length;

            if (Req.ContentLength == 0)
            {
                State = ParseState::ParseComplete;
                return ParseResult::Complete;
            }
            else
            {
                BytesRemaining = Req.ContentLength;
                State = ParseState::BodyFixedLength;
                return ParseResult::OK;
            }
        }
        catch (const std::exception& E)
        {
            std::cerr << "Error while converting from string to integer: " << E.what() << "\n";
            return ParseResult::Error;
        }
    }

    State = ParseState::ParseComplete;
    return ParseResult::Complete;
}
