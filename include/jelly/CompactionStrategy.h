#pragma once

namespace jelly
{

	enum CompactionStrategy : uint32_t
	{
		COMPACTION_STRATEGY_NONE,		// Don't do anything
		COMPACTION_STRATEGY_SMALLEST,	// Two smallest stores will be compacted
		COMPACTION_STRATEGY_NEWEST,		// Two newest stores will be compacted (this is really only useful for certain tests)

		NUM_COMPACTION_STRATEGIES
	};

}