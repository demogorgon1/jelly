#pragma once

#include <random>

namespace jelly::Test::Sim
{

	struct PlayerBlob
	{
		static constexpr size_t DATA_SIZE = 1024;

		PlayerBlob()
			: m_id(0)
		{
			memset(m_someData, 0, sizeof(m_someData));
		}

		void
		GenerateSomeData(
			std::mt19937&	aRandom)
		{
			for(size_t i = 0; i < DATA_SIZE; i++)
				m_someData[i] = 'a' + aRandom() % ('z' - 'a');
		}

		void
		ToBlob(
			Blob&			aBlob) const
		{
			aBlob.SetSize(DATA_SIZE + sizeof(m_id));
			memcpy(aBlob.GetBuffer(), &m_id, sizeof(m_id));
			memcpy((uint8_t*)aBlob.GetBuffer() + sizeof(m_id), m_someData, DATA_SIZE);
		}

		void
		FromBlob(
			const Blob&		aBlob)
		{
			JELLY_ASSERT(aBlob.GetSize() == DATA_SIZE + sizeof(m_id));
			memcpy(&m_id, aBlob.GetBuffer(), sizeof(m_id));
			memcpy(m_someData, (const uint8_t*)aBlob.GetBuffer() + sizeof(m_id), DATA_SIZE);
		}

		// Public data
		uint32_t	m_id;
		char		m_someData[DATA_SIZE];
	};

}