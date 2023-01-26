#pragma once

#include "NodeServer.h"

namespace jelly::Test::Sim
{

	namespace LockServer
	{
		typedef LockMetaData::StaticSingleBlob<4> LockMetaDataType;

		typedef LockNode<UIntKey<uint32_t>, UIntLock<uint32_t>, LockMetaDataType, UIntKey<uint32_t>::Hasher> LockNodeType;

		typedef NodeServer<LockNodeType, NODE_SERVER_TYPE_BLOB> LockServerType;

	}

}
