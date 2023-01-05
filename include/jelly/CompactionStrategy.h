#pragma once

namespace jelly
{

	enum CompactionStrategy
	{
		COMPACTION_STRATEGY_SMALLEST,	// Two smallest stores will be compacted
		COMPACTION_STRATEGY_NEWEST		// Two newest stores will be compacted (this is really only useful for certain tests)
	};

}