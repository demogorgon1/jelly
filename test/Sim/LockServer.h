#pragma once

#include "NodeServer.h"

namespace jelly::Test::Sim
{

	namespace LockServer
	{
		typedef MetaData::LockStaticSingleBlob<4> LockMetaDataType;

		typedef LockNode<UIntKey<uint32_t>, UIntLock<uint32_t>, LockMetaDataType> LockNodeType;

		typedef NodeServer<LockNodeType, NODE_SERVER_TYPE_BLOB> LockServerType;

	}

}
