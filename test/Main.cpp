#include <stdlib.h>

#include "AllTests.h"
#include "Config.h"

int
main(
	int		aNumArgs,
	char**	aArgs)
{	
	jelly::Test::Config config;
	config.InitFromCommandLine(aNumArgs, aArgs);

	config.m_simTest = true;
	config.m_simCSVOutput = "test.csv";
	config.m_simCSVOutputColumns = { "C_CONNECTED", "C_WAITING_FOR_CONNECTION", "C_WAITING_FOR_CONNECTION_cur_avg", "G_WAITING_FOR_BLOB_SET_cur_avg" };
	config.m_simNumClients = 200000;
	config.m_simNumClientThreads = 4;
	config.m_simSetIntervalMS = 60 * 1000;
	config.m_simClientStartIntervalMS = 1;
	config.m_simNumBlobServers = 4;
	config.m_simNumBlobServerThreads = 4;
	config.m_simNumGameServers = 4;
	config.m_simNumGameServerThreads = 4;

	jelly::Test::AllTests::Run(".", &config);

	return EXIT_SUCCESS;
}