#include <iostream>
#include <Parser.h>
#include <Utils.h>

HTTP::Parser::Parser()
{
    Reset();
}

void HTTP::Parser::Reset()
{
    Req = {};
    Req.Method = "";
    Req.URI = "";
    Req.Path = "";
    Req.Query = "";
    Req.Version = "";
    Req.Headers = {};
    Req.Trailers = {};
    Req.Body = "";
    Req.ContentLength = -1;
    Req.IsChunked = false;
    Req.IsComplete = false;
    Req.State = ParseState::RequestLineStart;

    State = ParseState::RequestLineStart;
    Buffer = "";
    CurrentHeaderName = "";
    CurrentHeaderValue = "";
    CurrentChunk = {};
    ChunkedBody = "";
    Position = 0;
    BytesRemaining = 0;
}

int HTTP::Parser::Parse(std::string Data, size_t DataLength)
{
    int BytesConsumed = 0;
    Buffer += Data.substr(0, DataLength);

    while (true)
    {
        ParseResult Result = ParseByState();

        if (Result == ParseResult::Incomplete) { break; }
        else if (Result == ParseResult::Error)
        {
            Req.State = State;
            return -1;
        }
        else if (Result == ParseResult::Complete)
        {
            Req.IsComplete = true;
            Req.State = ParseComplete;
            return BytesConsumed;
        }

        BytesConsumed = Position;
    }

    if (Position > 0)
    {
        Buffer.erase(0, Position);
        Position = 0;
    }

    return BytesConsumed;
}

HTTP::ParseResult HTTP::Parser::ParseByState()
{
    switch (State)
    {
    case RequestLineStart: { return ParseRequestLineStart(); break; }
    case RequestLineMethod: { return ParseRequestLineMethod(); break; }
    case RequestLineURI: { return ParseRequestLineURI(); break; }
    case RequestLineVersion: { return ParseRequestLineVersion(); break; }
    case RequestLineCRLF: { return ParseRequestLineCRLF(); break; }
    case HeaderName: { return ParseHeaderName(); break; }
    case HeaderValueStart: { return ParseHeaderValueStart(); break; }
    case HeaderValue: { return ParseHeaderValue(); break; }
    case HeaderValueCRLF: { return ParseHeaderValueCRLF(); break; }
    case HeaderEndCRLF: { return ParseHeaderEndCRLF(); break; }
    case BodyFixedLength: { return ParseBodyFixedLength(); break; }
    case BodyChunkSize: { return ParseChunkSize(); break; }
    case BodyChunkExtension: { return ParseChunkExtension(); break; }
    case BodyChunkSizeCRLF: { return ParseChunkSizeCRLF(); break; }
    case BodyChunkData: { return ParseChunkData(); break; }
    case BodyChunkDataCRLF: { return ParseChunkDataCRLF(); break; }
    case BodyChunkTrailerName: { return ParseChunkTrailerName(); break; }
    case BodyChunkTrailerValue: { return ParseChunkTrailerValue(); break; }
    case BodyChunkTrailerCRLF: { return ParseChunkTrailerCRLF(); break; }
    case BodyChunkFinalCRLF: { return ParseChunkFinalCRLF(); break; }
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
    int MethodStart = Position;
    while (Position < Buffer.size())
    {
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

    int URIStart = Position;
    while (Position < Buffer.size())
    {
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
    // NGINX style: Accept multiple spaces but not tabs or newlines before URI
    while (Position < Buffer.size() && Buffer[Position] == ' ') { Position++; }

    int VersionStart = Position;
    while (Position < Buffer.size())
    {
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

    int NameStart = Position;
    while (Position < Buffer.size())
    {
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
    int ValueStart = Position;
    while(Position < Buffer.size())
    {
        char Byte = Buffer[Position];
        if (Byte == '\r')
        {
            CurrentHeaderValue = Buffer.substr(ValueStart, Position - ValueStart);
            State = ParseState::HeaderValueCRLF;
            return ParseResult::OK;
        }
        else if (Byte == '\0' or Byte == '\n') { return ParseResult::Error; }
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
        Req.Body = ChunkedBody;
        State = ParseState::ParseComplete;
        return ParseResult::Complete;
    }

    size_t Available = Buffer.size() - Position;
    int ToRead = std::min(Available, BytesRemaining);

    Req.Body += Buffer.substr(Position, ToRead);
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
    int SizeStart = Position;
    while (Position < Buffer.size())
    {
        char Byte = Buffer[Position];
        if (IsHexDigit(Byte)) { Position++; }
        else if (Byte == '\r')
        {
            std::string HexString = Buffer.substr(SizeStart, Position - SizeStart);
            int HexSize = ParseHex(HexString);
			
			if (HexSize < 0) { return ParseResult::Error; }
			else { CurrentChunk.Size = HexSize; }

            State = ParseState::BodyChunkSizeCRLF;
            return ParseResult::OK;
        }
        else if (Byte == ';')
        {
            std::string HexString = Buffer.substr(SizeStart, Position - SizeStart);
            int HexSize = ParseHex(HexString);
			
			if (HexSize < 0) { return ParseResult::Error; }
			else { CurrentChunk.Size = HexSize; }

            Position++;
            State = ParseState::BodyChunkExtension;
            return ParseResult::OK;
        }
        else { return ParseResult::Error; }
    }

    return Incomplete;
}

HTTP::ParseResult HTTP::Parser::ParseChunkExtension()
{
    while (Position < Buffer.size())
    {
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
    int Available = Buffer.size() - Position;
    int Needed = CurrentChunk.Size - CurrentChunk.BytesRead;
    int ToRead = std::min(Available, Needed);

    CurrentChunk.Data += Buffer.substr(Position, ToRead);
    ChunkedBody += Buffer.substr(Position, ToRead);

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

    int NameStart = Position;
    while (Position < Buffer.size())
    {
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

    int ValueStart = Position;
    while(Position < Buffer.size())
    {
        char Byte = Buffer[Position];
        if (Byte == '\r')
        {
            CurrentTrailerValue = Buffer.substr(ValueStart, Position - ValueStart);
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
    size_t CLIndex = 0;

    // Transfer encoding takes priority over content length
    for (size_t i = 0; i < Req.Headers.size(); i++)
    {
        if (Req.Headers[i].Key == "transfer-encoding")
        {
            std::string TransferEncoding = ToLower(Req.Headers[i].Value);
            if (TransferEncoding == "chunked")
            {
                Req.IsChunked = true;
                State = ParseState::BodyChunkSize;
                return ParseResult::OK;
            }
        }

        if (Req.Headers[i].Key == "content-length")
        {
            CLFlag = true;
            CLIndex = i;
        }
    }

    if (CLFlag)
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
            std::cerr << "Error while converting from string to int: " << E.what() << "\n";
            return ParseResult::Error;
        }
    }

    State = ParseState::ParseComplete;
    return ParseResult::Complete;
}
