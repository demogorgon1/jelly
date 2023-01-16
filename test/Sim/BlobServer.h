#pragma once

#include "NodeServer.h"

namespace jelly::Test::Sim
{

	namespace BlobServer
	{

		typedef BlobNode<UIntKey<uint32_t>, Blob<1>, UIntKey<uint32_t>::Hasher> BlobNodeType;

		typedef NodeServer<BlobNodeType, NODE_SERVER_TYPE_BLOB> BlobServerType;

	}

}