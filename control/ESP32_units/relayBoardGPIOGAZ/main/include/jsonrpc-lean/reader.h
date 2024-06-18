// This file is derived from xsonrpc Copyright (C) 2015 Erik Johansson <erik@ejohansson.se>
// This file is part of jsonrpc-lean, a c++11 JSON-RPC client/server library.
//
// Modifications and additions for jsonrpc-lean Copyright (C) 2015 Adriano Maia <tony@stark.im>
//

#ifndef JSONRPC_LEAN_READER_H
#define JSONRPC_LEAN_READER_H

namespace jsonrpc {

    class Request;
    class Response;
    class Value;

    class Reader {
    public:
        virtual ~Reader() {}

        virtual Request GetRequest() = 0;
        virtual Response GetResponse() = 0;
        virtual Value GetValue() = 0;
    };

} // namespace jsonrpc

#endif // JSONRPC_LEAN_READER_H
