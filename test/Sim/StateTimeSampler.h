#pragma once

namespace jelly::Test::Sim
{

	class StateTimeSampler
	{
	public:
		StateTimeSampler(
			uint32_t		aNumStates)
			: m_currentState(0)
		{
			m_definitions.resize((size_t)aNumStates, { UINT32_MAX, UINT32_MAX });
			m_stateTimeStamp = std::chrono::steady_clock::now();
		}

		~StateTimeSampler()
		{

		}

		void
		DefineState(
			uint32_t		aState,
			uint32_t		aIdTime,
			uint32_t		aIdCurTime)
		{
			JELLY_ASSERT((size_t)aState < m_definitions.size());
			m_definitions[aState] = Definition{ aIdTime, aIdCurTime };
		}

		void
		ChangeState(
			IStats*			aStats,
			uint32_t		aNewState)
		{
			std::chrono::time_point<std::chrono::steady_clock> currentTime = std::chrono::steady_clock::now();
			uint32_t millisecondsSpentInState = (uint32_t)std::chrono::duration_cast<std::chrono::microseconds>(currentTime - m_stateTimeStamp).count();

			JELLY_ASSERT((size_t)m_currentState < m_definitions.size());
			uint32_t statsId = m_definitions[m_currentState].m_idTime;

			if(statsId != UINT32_MAX)
				aStats->Emit(statsId, millisecondsSpentInState, Stat::TYPE_SAMPLER);

			m_stateTimeStamp = currentTime;
			m_currentState = aNewState;
		}

		uint32_t
		GetCurrentStateTime(
			std::chrono::time_point<std::chrono::steady_clock>	aCurrentTime) const
		{
			uint64_t t = (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(aCurrentTime - m_stateTimeStamp).count();
			return (uint32_t)std::min<uint64_t>(t, UINT32_MAX);
		}

		void
		EmitCurrentStateTime(
			IStats*												aStats,
			std::chrono::time_point<std::chrono::steady_clock>	aCurrentTime) const
		{
			JELLY_ASSERT((size_t)m_currentState < m_definitions.size());
			uint32_t statsId = m_definitions[m_currentState].m_idCurTime;

			if (statsId != UINT32_MAX)
				aStats->Emit(statsId, GetCurrentStateTime(aCurrentTime), Stat::TYPE_SAMPLER);
		}

	private:

		struct Definition
		{
			uint32_t	m_idTime;
			uint32_t	m_idCurTime;
		};

		std::chrono::time_point<std::chrono::steady_clock>	m_stateTimeStamp;
		uint32_t											m_currentState;
		std::vector<Definition>								m_definitions;
	};

}