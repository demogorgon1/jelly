#pragma once

namespace jelly
{

	class IStats
	{
	public:
		IStats()
		{

		}

		virtual			
		~IStats() 
		{
		}

		struct Counter
		{
			Counter()
				: m_value(0)
				, m_rate(0)
				, m_rateMA(0)
			{

			}

			uint64_t		m_value;
			uint64_t		m_rate;
			uint64_t		m_rateMA;
		};

		struct Sampler
		{
			Sampler()
				: m_avg(0)
				, m_min(0)
				, m_max(0)
				, m_count(0)
			{

			}

			uint64_t		m_avg;
			uint64_t		m_min;
			uint64_t		m_max;
			uint64_t		m_count;
		};

		struct Gauge
		{
			Gauge()
				: m_value(0)
			{

			}

			uint64_t		m_value;
		};

		template <typename _T>
		void
		AddCounter(
			uint32_t						aId,
			const _T&						aValue)
		{
			static_assert(std::is_unsigned<_T>::value);

			AddCounter_UInt64(aId, (uint64_t)aValue);
		}

		template <typename _T>
		void
		Sample(
			uint32_t						aId,
			const _T&						aValue)
		{
			static_assert(std::is_unsigned<_T>::value);

			Sample_UInt64(aId, (uint64_t)aValue);
		}

		template <typename _T>
		void
		SetGauge(
			uint32_t						aId,
			const _T&						aValue)
		{
			static_assert(std::is_unsigned<_T>::value);

			SetGauge_UInt64(aId, (uint64_t)aValue);
		}

		// Virtual methods
		virtual void				AddCounter_UInt64(
										uint32_t		/*aId*/,
										uint64_t		/*aValue*/) { }
		virtual void				Sample_UInt64(
										uint32_t		/*aId*/,
										uint64_t		/*aValue*/) { }
		virtual void				SetGauge_UInt64(
										uint32_t		/*aId*/,
										uint64_t		/*aValue*/) { }
		virtual void				Update() { }
		virtual Counter				GetCounter(
										uint32_t		/*aId*/) { return Counter(); }
		virtual Sampler				GetSampler(
										uint32_t		/*aId*/) { return Sampler(); }
		virtual Gauge				GetGauge(
										uint32_t		/*aId*/) { return Gauge(); }
	};

}