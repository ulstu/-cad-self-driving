// This file is part of jsonrpc-lean, a c++11 JSON-RPC client/server library.
//
// Copyright (C) 2015 Adriano Maia <tony@stark.im>
//

#ifndef JSONRPC_LEAN_JSONREQUESTDATA_H
#define JSONRPC_LEAN_JSONREQUESTDATA_H

#include "formatteddata.h"

#define RAPIDJSON_NO_SIZETYPEDEFINE
namespace rapidjson { typedef ::std::size_t SizeType; }

#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

namespace jsonrpc {

    class JsonFormattedData final : public FormattedData {

    private:
        rapidjson::StringBuffer myStringBuffer;

    public:
        JsonFormattedData() : Writer(myStringBuffer) {

        }

        const char* GetData() override {
            return myStringBuffer.GetString();
        }

        size_t GetSize() override {
            return myStringBuffer.GetSize();
        }

        rapidjson::Writer<rapidjson::StringBuffer> Writer;

    
        
    };

} // namespace jsonrpc

#endif // JSONRPC_LEAN_JSONREQUESTDATA_H