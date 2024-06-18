// This file is derived from xsonrpc Copyright (C) 2015 Erik Johansson <erik@ejohansson.se>
// This file is part of jsonrpc-lean, a c++11 JSON-RPC client/server library.
//
// Modifications and additions for jsonrpc-lean Copyright (C) 2015 Adriano Maia <tony@stark.im>
//

#ifndef JSONRPC_LEAN_FORMATHANDLER_H
#define JSONRPC_LEAN_FORMATHANDLER_H

#include <memory>
#include <string>

namespace jsonrpc {

    class Reader;
    class Writer;

    class FormatHandler {
    public:
        virtual ~FormatHandler() {}

        virtual bool CanHandleRequest(const std::string& contentType) = 0;
        virtual std::string GetContentType() = 0;
        virtual bool UsesId() = 0;
        virtual std::unique_ptr<Reader> CreateReader(const std::string& data) = 0;
        virtual std::unique_ptr<Writer> CreateWriter() = 0;
    };

} // namespace jsonrpc

#endif // JSONRPC_LEAN_FORMATHANDLER_H
