#include <jelly/API.h>

#include "AllTests.h"
#include "Config.h"

int
main(
	int		aNumArgs,
	char**	aArgs)
{	
	jelly::Test::Config config;
	config.InitFromCommandLine(aNumArgs, aArgs);

	config.m_writeTest = true;
	config.m_writeTestBlobSize = 10 * 1024;
	config.m_writeTestBlobCount = 20000;

	////config.m_hammerTest = true;
	//
	//config.m_simTest = true;
	//config.m_simCSVOutput = "test.csv";
	//
	//config.m_simCSVOutputColumns = 
	//{ 
	//	"g:gs_num_clients", 
	//	"g:client_limit",
	//	"s:avg:gs_c_waiting_for_blob_set_time", 
	//	"s:avg:perform_blob_compaction_time",
	//	"s:avg:apply_blob_compaction_time",
	//	"g:total_host_wal_size",
	//	"g:memory_usage",
	//	"g:total_host_store_size",
	//	"g:total_resident_blob_size",
	//	"g:c_waiting_for_connection_num",
	//	"c:rate:disk_write_blob_wal_bytes",
	//	"c:rate:disk_write_blob_store_bytes",
	//	"c:rate:disk_read_blob_wal_bytes",
	//	"c:rate:disk_read_blob_store_bytes"
	//};

	////config.m_simStdOut = 
	////{
	////	"compressed_blob_size",
	////	"uncompressed_blob_size"
	////};

	//config.m_simNumClients = 500000;
	//config.m_simStartClientLimit = 0;
	//config.m_simNumClientThreads = 2;
	//config.m_simSetIntervalMS = 10000;
	//config.m_simClientStartIntervalMS = 0;
	//config.m_simNumBlobServers = 2;
	//config.m_simNumBlobServerThreads = 2;
	//config.m_simNumGameServers = 2;
	//config.m_simNumGameServerThreads = 2;
	//config.m_simNumSharedWorkerThreads = 2;
	//config.m_simConfigSource.SetString("max_resident_blob_size", "0");
	//config.m_simConfigSource.SetString("compression_method", "none");

	////config.m_writeTest = true;
	////config.m_writeTestBlobSize = 10 * 1024;
	//////config.m_writeTestBlobCount = 200000;
	////config.m_writeTestBlobCount = 100000;
	////config.m_writeTestBufferCompressionLevel = 1;

	////config.m_readTest = true;
	////config.m_readTestBlobCount = config.m_writeTestBlobCount;
	////config.m_readTestBlobCountMemoryLimit = 0;

	jelly::Test::AllTests::Run(".", &config);

	return EXIT_SUCCESS;
}