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
			std::mt19937&				aRandom)
		{
			for(size_t i = 0; i < DATA_SIZE; i++)
				m_someData[i] = 'a' + aRandom() % ('z' - 'a');
		}

		IBuffer*
		AsBlob() const
		{
			std::unique_ptr<Buffer<1>> buffer = std::make_unique<Buffer<1>>();
			buffer->SetSize(DATA_SIZE + sizeof(m_id));
			memcpy(buffer->GetPointer(), &m_id, sizeof(m_id));
			memcpy((uint8_t*)buffer->GetPointer() + sizeof(m_id), m_someData, DATA_SIZE);
			return buffer.release();
		}

		void
		FromBlob(
			const IBuffer*				aBlob)
		{
			JELLY_ASSERT(aBlob->GetSize() == DATA_SIZE + sizeof(m_id));
			memcpy(&m_id, aBlob->GetPointer(), sizeof(m_id));
			memcpy(m_someData, (const uint8_t*)aBlob->GetPointer() + sizeof(m_id), DATA_SIZE);
		}

		// Public data
		uint32_t	m_id;
		char		m_someData[DATA_SIZE];
	};

}