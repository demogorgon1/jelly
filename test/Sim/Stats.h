#pragma once

namespace jelly::Test::Sim
{

	namespace Stats
	{
		
		enum Id : uint32_t
		{
			// GameServer
			ID_GS_NUM_CLIENTS = Stat::NUM_IDS, 
			ID_GS_UPDATE_TIME,
			ID_GS_C_INIT_TIME,
			ID_GS_C_NEED_LOCK_TIME,
			ID_GS_C_WAITING_FOR_LOCK_TIME,
			ID_GS_C_NEED_BLOB_TIME,
			ID_GS_C_WAITING_FOR_BLOB_GET_TIME,
			ID_GS_C_CONNECTED_TIME,
			ID_GS_C_WAITING_FOR_BLOB_SET_TIME,
			ID_GS_C_INIT_CUR_TIME,
			ID_GS_C_NEED_LOCK_CUR_TIME,
			ID_GS_C_WAITING_FOR_LOCK_CUR_TIME,
			ID_GS_C_NEED_BLOB_CUR_TIME,
			ID_GS_C_WAITING_FOR_BLOB_GET_CUR_TIME,
			ID_GS_C_CONNECTED_CUR_TIME,
			ID_GS_C_WAITING_FOR_BLOB_SET_CUR_TIME,
			ID_GS_C_INIT_NUM,
			ID_GS_C_NEED_LOCK_NUM,
			ID_GS_C_WAITING_FOR_LOCK_NUM,
			ID_GS_C_NEED_BLOB_NUM,
			ID_GS_C_WAITING_FOR_BLOB_GET_NUM,
			ID_GS_C_CONNECTED_NUM,
			ID_GS_C_WAITING_FOR_BLOB_SET_NUM,

			// LockServer
			ID_LS_INIT_TIME,
			ID_LS_RUNNING_TIME,
			ID_LS_INIT_CUR_TIME,
			ID_LS_RUNNING_CUR_TIME,
			ID_LS_INIT_NUM,
			ID_LS_RUNNING_NUM,

			// BlobServer
			ID_BS_INIT_TIME,
			ID_BS_RUNNING_TIME,
			ID_BS_INIT_CUR_TIME,
			ID_BS_RUNNING_CUR_TIME,
			ID_BS_INIT_NUM,
			ID_BS_RUNNING_NUM,

			// Client
			ID_C_INIT_TIME,
			ID_C_WAITING_TO_CONNECT_TIME,
			ID_C_WAITING_FOR_CONNECTION_TIME,
			ID_C_CONNECTED_TIME,
			ID_C_DISCONNECTED_TIME,
			ID_C_INIT_CUR_TIME,
			ID_C_WAITING_TO_CONNECT_CUR_TIME,
			ID_C_WAITING_FOR_CONNECTION_CUR_TIME,
			ID_C_CONNECTED_CUR_TIME,
			ID_C_DISCONNECTED_CUR_TIME,
			ID_C_INIT_NUM,
			ID_C_WAITING_TO_CONNECT_NUM,
			ID_C_WAITING_FOR_CONNECTION_NUM,
			ID_C_CONNECTED_NUM,
			ID_C_DISCONNECTED_NUM,

			NUM_IDS
		};


		// IMPORTANT: Order must match the Id enum
		static const Stat::Info EXTRA_APPLICATION_STATS[] =
		{
			//                                          | m_type             | m_id                              | m_cRateMA | m_sHistBuckets
			//------------------------------------------+--------------------+-----------------------------------+-----------+-----------------------------------------
			/* ID_GS_NUM_CLIENTS */					    { Stat::TYPE_GAUGE,   "gs_num_clients",                    0,          {} },
			/* ID_GS_UPDATE_TIME */                     { Stat::TYPE_SAMPLER, "gs_update_time",                    0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_GS_C_INIT_TIME */                     { Stat::TYPE_SAMPLER, "gs_c_init_time",                    0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_GS_C_NEED_LOCK_TIME */                { Stat::TYPE_SAMPLER, "gs_c_need_lock_time",               0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_GS_C_WAITING_FOR_LOCK_TIME */         { Stat::TYPE_SAMPLER, "gs_c_waiting_for_lock_time",        0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_GS_C_NEED_BLOB_TIME */                { Stat::TYPE_SAMPLER, "gs_c_need_blob_time",               0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_GS_C_WAITING_FOR_BLOB_GET_TIME */     { Stat::TYPE_SAMPLER, "gs_c_waiting_for_blob_get_time",    0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_GS_C_CONNECTED_TIME */                { Stat::TYPE_SAMPLER, "gs_c_connected_time",               0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_GS_C_WAITING_FOR_BLOB_SET_TIME */     { Stat::TYPE_SAMPLER, "gs_c_waiting_for_blob_set_time",    0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_GS_C_INIT_CUR_TIME */                 { Stat::TYPE_SAMPLER, "gs_c_init_cur_time",                0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_GS_C_NEED_LOCK_CUR_TIME */            { Stat::TYPE_SAMPLER, "gs_c_need_lock_cur_time",           0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_GS_C_WAITING_FOR_LOCK_CUR_TIME */     { Stat::TYPE_SAMPLER, "gs_c_waiting_for_lock_cur_time",    0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_GS_C_NEED_BLOB_CUR_TIME */            { Stat::TYPE_SAMPLER, "gs_c_need_blob_cur_time",           0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_GS_C_WAITING_FOR_BLOB_GET_CUR_TIME */ { Stat::TYPE_SAMPLER, "gs_c_waiting_for_blob_get_cur_time",0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_GS_C_CONNECTED_CUR_TIME */            { Stat::TYPE_SAMPLER, "gs_c_connected_cur_time",           0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_GS_C_WAITING_FOR_BLOB_SET_CUR_TIME */ { Stat::TYPE_SAMPLER, "gs_c_waiting_for_blob_set_cur_time",0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_GS_C_INIT_NUM */					    { Stat::TYPE_GAUGE,	  "gs_c_init_num",                     0,          {} },
			/* ID_GS_C_NEED_LOCK_NUM */				    { Stat::TYPE_GAUGE,	  "gs_c_need_lock_num",                0,          {} },
			/* ID_GS_C_WAITING_FOR_LOCK_NUM */		    { Stat::TYPE_GAUGE,	  "gs_c_waiting_for_lock_num",         0,          {} },
			/* ID_GS_C_NEED_BLOB_NUM */				    { Stat::TYPE_GAUGE,	  "gs_c_need_blob_num",                0,          {} },
			/* ID_GS_C_WAITING_FOR_BLOB_GET_NUM */	    { Stat::TYPE_GAUGE,	  "gs_c_waiting_for_blob_get_num",     0,          {} },
			/* ID_GS_C_CONNECTED_NUM */				    { Stat::TYPE_GAUGE,	  "gs_c_connected_num",                0,          {} },
			/* ID_GS_C_WAITING_FOR_BLOB_SET_NUM */	    { Stat::TYPE_GAUGE,	  "gs_c_waiting_for_blob_set_num",     0,          {} },
													    													      
			/* ID_LS_INIT_TIME */					    { Stat::TYPE_SAMPLER, "ls_init_time",                      0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_LS_RUNNING_TIME */				    { Stat::TYPE_SAMPLER, "ls_running_time",                   0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_LS_INIT_CUR_TIME */					{ Stat::TYPE_SAMPLER, "ls_init_cur_time",                  0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_LS_RUNNING_CUR_TIME */				{ Stat::TYPE_SAMPLER, "ls_running_cur_time",               0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_LS_INIT_NUM */					    { Stat::TYPE_GAUGE,   "ls_init_num",                       0,          {} },
			/* ID_LS_RUNNING_NUM */					    { Stat::TYPE_GAUGE,   "ls_running_num",                    0,          {} },
													    													     	           
			/* ID_BS_INIT_TIME */					    { Stat::TYPE_SAMPLER, "bs_init_time",                      0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_BS_RUNNING_TIME */				    { Stat::TYPE_SAMPLER, "bs_running_time",                   0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_BS_INIT_CUR_TIME */				    { Stat::TYPE_SAMPLER, "bs_init_cur_time",                  0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_BS_RUNNING_CUR_TIME */				{ Stat::TYPE_SAMPLER, "bs_running_cur_time",               0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_BS_INIT_NUM */					    { Stat::TYPE_GAUGE,   "bs_init_num",                       0,          {} },
			/* ID_BS_RUNNING_NUM */					    { Stat::TYPE_GAUGE,   "bs_running_num",                    0,          {} },
													    													       	           
			/* ID_C_INIT_TIME */					    { Stat::TYPE_SAMPLER, "c_init_time",                       0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_C_WAITING_TO_CONNECT_TIME */		    { Stat::TYPE_SAMPLER, "c_waiting_to_connect_time",         0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_C_WAITING_FOR_CONNECTION_TIME */	    { Stat::TYPE_SAMPLER, "c_waiting_for_connection_time",     0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_C_CONNECTED_TIME */				    { Stat::TYPE_SAMPLER, "c_connected_time",                  0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_C_DISCONNECTED_TIME */			    { Stat::TYPE_SAMPLER, "c_disconnected_time",               0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_C_INIT_CUR_TIME */					{ Stat::TYPE_SAMPLER, "c_init_cur_time",                   0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_C_WAITING_TO_CONNECT_CUR_TIME */		{ Stat::TYPE_SAMPLER, "c_waiting_to_connect_cur_time",     0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_C_WAITING_FOR_CONNECTION_CUR_TIME */	{ Stat::TYPE_SAMPLER, "c_waiting_for_connection_cur_time", 0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_C_CONNECTED_CUR_TIME */				{ Stat::TYPE_SAMPLER, "c_connected_cur_time",              0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_C_DISCONNECTED_CUR_TIME */			{ Stat::TYPE_SAMPLER, "c_disconnected_cur_time",           0,          Stat::TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_C_INIT_NUM */						    { Stat::TYPE_GAUGE,   "c_init_num",                        0,          {} },
			/* ID_C_WAITING_TO_CONNECT_NUM */		    { Stat::TYPE_GAUGE,   "c_waiting_to_connect_num",          0,          {} },
			/* ID_C_WAITING_FOR_CONNECTION_NUM */	    { Stat::TYPE_GAUGE,   "c_waiting_for_connection_num",      0,          {} },
			/* ID_C_CONNECTED_NUM */				    { Stat::TYPE_GAUGE,   "c_connected_num",                   0,          {} },
			/* ID_C_DISCONNECTED_NUM */				    { Stat::TYPE_GAUGE,   "c_disconnected_num",                0,          {} }

		};

		static_assert(sizeof(EXTRA_APPLICATION_STATS) == sizeof(Stat::Info) * ((size_t)NUM_IDS - (size_t)Stat::NUM_IDS));
	
		inline const Stat::Info* 
		GetExtraApplicationStats() 
		{
			return &EXTRA_APPLICATION_STATS[0];
		}

		inline uint32_t
		GetExtraApplicationStatsCount()
		{
			return (uint32_t)NUM_IDS - (uint32_t)Stat::NUM_IDS;
		}

	}

}
