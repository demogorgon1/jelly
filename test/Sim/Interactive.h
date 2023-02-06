#pragma once

namespace jelly::Test::Sim
{

	class Network;

	class Interactive
	{
	public:
						Interactive(
							Network*			aNetwork);
						~Interactive();

		bool			Update();

	private:

		Network*						m_network;
		std::unique_ptr<std::thread>	m_inputThread;

		std::mutex						m_inputLock;
		std::vector<std::string>		m_input;

		static void		_ThreadMain(
							Interactive*		aInteractive);
	};

}