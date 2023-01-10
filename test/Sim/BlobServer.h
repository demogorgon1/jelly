#pragma once

#include <jelly/Blob.h>
#include <jelly/UIntKey.h>

#include "NodeServer.h"

namespace jelly::Test::Sim
{

	namespace BlobServer
	{

		typedef BlobNode<UIntKey<uint32_t>, Blob, UIntKey<uint32_t>::Hasher> BlobNodeType;

		typedef NodeServer<BlobNodeType, NODE_SERVER_TYPE_BLOB> BlobServerType;

	}

}