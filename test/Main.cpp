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

	//config.m_hammerTest = true;
	
	config.m_simTest = true;
	config.m_simCSVOutput = "test.csv";
	config.m_simCSVOutputColumns = { "g:gs_num_clients", "s:avg:gs_c_waiting_for_blob_set_time" };
	config.m_simNumClients = 20000;
	config.m_simNumClientThreads = 2;
	config.m_simSetIntervalMS = 1000;
	config.m_simClientStartIntervalMS = 1;
	config.m_simNumBlobServers = 2;
	config.m_simNumBlobServerThreads = 2;
	config.m_simNumGameServers = 2;
	config.m_simNumGameServerThreads = 2;
	config.m_simTestStdOut = true;

	//config.m_writeTest = true;
	//config.m_writeTestBlobSize = 10 * 1024;
	////config.m_writeTestBlobCount = 200000;
	//config.m_writeTestBlobCount = 100000;
	//config.m_writeTestBufferCompressionLevel = 1;

	//config.m_readTest = true;
	//config.m_readTestBlobCount = config.m_writeTestBlobCount;
	//config.m_readTestBlobCountMemoryLimit = 0;

	jelly::Test::AllTests::Run(".", &config);

	return EXIT_SUCCESS;
}