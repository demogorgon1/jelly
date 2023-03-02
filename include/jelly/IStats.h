#pragma once

#include "Stat.h"

namespace jelly
{

	/**
	 * \brief Interface for emitting and querying database statistics. 
	 *
	 * Statistics come in 3 flavors:
	 * 
	 * Statistic | <!-- -->
	 * ----------|-----------------------------------------------------------------------------
	 * Counter   | An incrementing number, for example number of bytes being written to disk.
	 * Sampler   | Samples a value of some number, for example duration of some operation.
	 * Gauge     | A number, for example current number of WAL files on disk.
	 * 
	 * Statistics are reset with every call to Update().
	 * 
	 * \see Stats.h
	 */
	class IStats
	{
	public:
		IStats() noexcept
		{

		}

		virtual			
		~IStats() 
		{
		}

		/**
		 * \brief Describes the state of a counter. 
		 */
		struct Counter
		{
			Counter() noexcept
				: m_value(0)
				, m_rate(0)
				, m_rateMA(0)
			{

			}

			uint64_t		m_value;	///< Current counter value.
			uint64_t		m_rate;		///< Rate.
			uint64_t		m_rateMA;	///< Moving average filtered rate.
		};

		/**
		 * \brief Describes the state of a sampler.
		 */
		struct Sampler
		{
			Sampler() noexcept
				: m_avg(0)
				, m_min(0)
				, m_max(0)
				, m_count(0)
			{

			}

			uint64_t		m_avg;		///< Average sample value.
			uint64_t		m_min;		///< Minimum sample value.
			uint64_t		m_max;		///< Maximum sample value.
			uint64_t		m_count;	///< Number of samples.
		};

		/**
		 * \brief Histogram of a sampler
		 */
		struct SamplerHistogramView
		{
			SamplerHistogramView(
				const std::vector<uint64_t>*	aBuckets = NULL,
				const uint32_t*					aSampleCountsPerBucket = NULL)
				: m_buckets(aBuckets)
				, m_sampleCountsPerBucket(aSampleCountsPerBucket)
			{

			}

			const std::vector<uint64_t>*	m_buckets;					///< Description of buckets
			const uint32_t*					m_sampleCountsPerBucket;	///< Samples per bucket
		};

		/**
		 * \brief Describes the state of a gauge.
		 */
		struct Gauge
		{
			Gauge() noexcept
				: m_value(0)
			{

			}

			uint64_t		m_value;	///< Value of the gauge.
		};

		//! Emit a statistic specified by aId. 
		template <typename _T>
		void
		Emit(
			uint32_t							aId,
			const _T&							aValue,
			const std::optional<Stat::Type>&	aExpectedType = std::optional<Stat::Type>()) noexcept
		{
			static_assert(std::is_unsigned<_T>::value);

			Emit_UInt64(aId, (uint64_t)aValue, aExpectedType);
		}

		//---------------------------------------------------------------------------
		// Virtual methods

		virtual void					Emit_UInt64(
											uint32_t							/*aId*/,
											uint64_t							/*aValue*/,
											const std::optional<Stat::Type>&	/*aExpectedType*/) noexcept { }

		//! Update statistics. Counters and gauges will be reset.
		virtual void					Update() { }

		//! Query a counter.
		virtual Counter					GetCounter(
											uint32_t							/*aId*/) const { return Counter(); }

		//! Query a sampler.
		virtual Sampler					GetSampler(
											uint32_t							/*aId*/) const { return Sampler(); }

		//! Query a sampler histogram.
		virtual SamplerHistogramView	GetSamplerHistogramView(
											uint32_t							/*aId*/) const { return SamplerHistogramView(); }

		//! Query a gauge.		 
		virtual Gauge					GetGauge(
											uint32_t							/*aId*/) const { return Gauge(); }

		//! Get a statistic id from a string name.
		virtual uint32_t				GetIdByString(
											const char*							/*aString*/) const { return UINT32_MAX; }

		//! Get statistic info by id.
		virtual const Stat::Info*		GetInfo(
											uint32_t							/*aId*/) const { JELLY_ASSERT(false); return NULL; }
	};

}