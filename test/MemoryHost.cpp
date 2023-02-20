#include <jelly/API.h>

#include "MemoryHost.h"

namespace jelly
{

	namespace
	{

		struct DummyStats
			: public IStats
		{
			virtual ~DummyStats() {}
		};
		
		struct MemoryBuffer
		{
			static const size_t MAX_SIZE = 32768;

			MemoryBuffer()
				: m_size(0)
				, m_next(NULL)
			{
				memset(m_data, 0, sizeof(m_data));
			}

			// Public data
			uint8_t			m_data[MAX_SIZE];
			size_t			m_size;
			MemoryBuffer*	m_next;
		};

		struct BufferList
		{
			BufferList()
				: m_head(NULL)
				, m_tail(NULL)
				, m_totalBytes(0)
			{
				
			}

			~BufferList()
			{
				while(m_head != NULL)
				{
					MemoryBuffer* next = m_head->m_next;
					delete m_head;
					m_head = next;
				}
			}

			void
			CreateBufferAtTail()
			{
				if(m_head == NULL)
				{
					JELLY_ASSERT(m_tail == NULL);
					m_head = new MemoryBuffer();
					m_tail = m_head;
				}
				else
				{
					JELLY_ASSERT(m_head != NULL);
					m_tail->m_next = new MemoryBuffer();
					m_tail = m_tail->m_next;
				}
			}

			void
			GetBufferByOffset(
				size_t			aOffset,
				const MemoryBuffer*&	aOutBuffer,
				size_t&			aOutBufferOffset)
			{
				if(m_array.size() == 0)
				{
					// Make array for easy offset lookup
					for(const MemoryBuffer* buffer = m_head; buffer != NULL; buffer = buffer->m_next)
						m_array.push_back(buffer);

					if(m_array.size() > 1)
					{
						// If more than one buffer, all of them except last should be max size
						for(size_t i = 0; i < m_array.size() - 1; i++)
							JELLY_ASSERT(m_array[i]->m_size == MemoryBuffer::MAX_SIZE);
					}
				}

				size_t i = aOffset / MemoryBuffer::MAX_SIZE;
				JELLY_ASSERT(i < m_array.size());
				size_t j = aOffset % MemoryBuffer::MAX_SIZE;
				JELLY_ASSERT(j < m_array[i]->m_size);
				
				aOutBuffer = m_array[i];
				aOutBufferOffset = j;
			}
			
			// Public data
			MemoryBuffer*						m_head;
			MemoryBuffer*						m_tail;
			size_t								m_totalBytes;
			std::vector<const MemoryBuffer*>	m_array;
		};

		struct BufferListWriter
			: public IWriter
		{
			BufferListWriter(
				BufferList*	aBufferList)
				: m_bufferList(aBufferList)
			{
				
			}

			virtual
			~BufferListWriter()
			{

			}

			// IWriter implementation
			size_t	
			Write(
				const void* aBuffer,
				size_t		aBufferSize) override
			{
				const uint8_t* p = (const uint8_t*)aBuffer;
				size_t remaining = aBufferSize;

				while(remaining > 0)
				{
					size_t spaceLeft = m_bufferList->m_tail != NULL ? (MemoryBuffer::MAX_SIZE - m_bufferList->m_tail->m_size) : 0;
					if(spaceLeft == 0)
					{
						m_bufferList->CreateBufferAtTail();
						spaceLeft = MemoryBuffer::MAX_SIZE;
					}

					size_t toCopy = std::min<size_t>(spaceLeft, remaining);
					memcpy(m_bufferList->m_tail->m_data + m_bufferList->m_tail->m_size, p, toCopy);
					m_bufferList->m_tail->m_size += toCopy;
					p += toCopy;
					remaining -= toCopy;
				}

				m_bufferList->m_totalBytes += aBufferSize;

				return aBufferSize;
			}

			size_t
			GetTotalBytesWritten() const override
			{
				return m_bufferList->m_totalBytes;
			}

			// Public data
			BufferList*			m_bufferList;
		};

		struct BufferListReader
			: public IReader
		{
			BufferListReader(
				const MemoryBuffer*	aStartBuffer,
				size_t			aStartBufferOffset)
				: m_currentBuffer(aStartBuffer)
				, m_currentBufferOffset(aStartBufferOffset)
				, m_globalReadOffset(0)
			{
			}

			virtual
			~BufferListReader()
			{

			}

			// IReader implementation
			size_t	
			Read(
				void*				aBuffer,
				size_t				aBufferSize) override
			{
				uint8_t* p = (uint8_t*)aBuffer;
				size_t remaining = aBufferSize;
				size_t totalRead = 0;

				while(remaining > 0 && m_currentBuffer != NULL)
				{
					JELLY_ASSERT(m_currentBufferOffset <= m_currentBuffer->m_size);
					size_t toCopy = std::min<size_t>(m_currentBuffer->m_size - m_currentBufferOffset, remaining);

					if(toCopy > 0)
					{
						memcpy(p, m_currentBuffer->m_data + m_currentBufferOffset, toCopy);
						m_currentBufferOffset += toCopy;
						p += toCopy;
						totalRead += toCopy;
						remaining -= toCopy;
					}

					if(m_currentBufferOffset == m_currentBuffer->m_size)
					{
						m_currentBuffer = m_currentBuffer->m_next;
						m_currentBufferOffset = 0;
					}
				}

				m_globalReadOffset += totalRead;

				return totalRead;
			}

			size_t
			GetTotalBytesRead() const override
			{
				return m_globalReadOffset;
			}

			// Public data
			const MemoryBuffer*		m_currentBuffer;
			size_t					m_currentBufferOffset;
			size_t					m_globalReadOffset;
		};

	}

	namespace Test
	{
	
		struct MemoryHost::Store
		{
			Store()
			{
			}

			~Store()
			{

			}

			IFileStreamReader*
			ReadStream()
			{
				return new FileStreamReader(&m_data);
			}

			IStoreBlobReader*
			Read()
			{
				return new StoreBlobReader(&m_data);
			}

			IStoreWriter*
			Write()
			{
				return new StoreWriter(&m_data);
			}

			// Public data
			BufferList		m_data;

			struct FileStreamReader : public IFileStreamReader
			{
				FileStreamReader(
					const BufferList*		aBufferList)
					: m_bufferListReader(aBufferList->m_head, 0)
				{
				}

				virtual
				~FileStreamReader()
				{

				}

				// IReader implementation
				size_t	
				Read(
					void*				aBuffer,
					size_t				aBufferSize) override
				{
					return m_bufferListReader.Read(aBuffer, aBufferSize);
				}

				size_t	
				GetTotalBytesRead() const override
				{
					return m_bufferListReader.GetTotalBytesRead();
				}

				// IFileStreamReader implementation
				bool	
				IsEnd() const override
				{
					return m_bufferListReader.m_currentBuffer == NULL;
				}

				// Public data
				BufferListReader m_bufferListReader;
			};

			struct StoreBlobReader : public IStoreBlobReader
			{
				StoreBlobReader(
					BufferList*				aBufferList)
					: m_bufferList(aBufferList)
				{

				}

				virtual
				~StoreBlobReader()
				{

				}

				// IStoreBlobReader implementation
				void	
				ReadItemBlob(
					size_t				aOffset,
					ItemBase*			aItem) override
				{
					JELLY_ASSERT(m_bufferList != NULL);

					std::unique_ptr<IBuffer> blobBuffer = std::make_unique<Buffer<1>>();
					blobBuffer->SetSize(aItem->GetStoredBlobSize());

					const MemoryBuffer* buffer;
					size_t bufferStartOffset;
					m_bufferList->GetBufferByOffset(aOffset, buffer, bufferStartOffset);

					BufferListReader reader(buffer, bufferStartOffset);
					if(reader.Read(blobBuffer->GetPointer(), blobBuffer->GetSize()) != blobBuffer->GetSize())
						JELLY_ASSERT(false);

					aItem->UpdateBlobBuffer(blobBuffer);
				}

				void	
				Close() override
				{
					JELLY_ASSERT(m_bufferList != NULL);
					m_bufferList = NULL;
				}

				// Public data
				BufferList*		m_bufferList;
			};

			struct StoreWriter : public IStoreWriter
			{
				StoreWriter(
					BufferList*	aBufferList)
					: m_bufferList(aBufferList)
				{
				}

				// IStoreWriter implementation
				size_t	
				WriteItem(
					const ItemBase*					aItem) override
				{
					size_t offset = m_bufferList->m_totalBytes;

					BufferListWriter writer(m_bufferList);
					aItem->Write(&writer);

					return offset;
				}

				void
				Flush() override
				{
					// Do nothing
				}

				// Public data
				BufferList* m_bufferList;
			};
		};

		struct MemoryHost::WAL
		{
			WAL()
			{

			}

			~WAL()
			{
				
			}

			IFileStreamReader*
			Read()
			{
				return new FileStreamReader(&m_data);
			}

			IWALWriter*
			Write()
			{
				return new WALWriter(&m_data);
			}

			// Public data	
			BufferList		m_data;
			
			struct FileStreamReader : public IFileStreamReader
			{
				FileStreamReader(
					const BufferList*		aBufferList)
					: m_bufferListReader(aBufferList->m_head, 0)
				{
				}

				virtual
				~FileStreamReader()
				{

				}

				// IReader implementation
				size_t	
				Read(
					void*				aBuffer,
					size_t				aBufferSize) override
				{
					return m_bufferListReader.Read(aBuffer, aBufferSize);
				}

				size_t
				GetTotalBytesRead() const override
				{
					return m_bufferListReader.GetTotalBytesRead();
				}

				// IFileStreamReader implementation
				bool	
				IsEnd() const override
				{
					return m_bufferListReader.m_currentBuffer == NULL;
				}			

				// Public data
				BufferListReader m_bufferListReader;
			};

			struct WALWriter : public IWALWriter
			{
				WALWriter(
					BufferList*		aBufferList)
					: m_bufferList(aBufferList)
				{
				}

				// IWALWriter implementation
				size_t	
				GetSize() const override
				{
					return m_bufferList->m_totalBytes;
				}
				
				void	
				WriteItem(
					const ItemBase*		aItem,
					CompletionEvent*	aCompletionEvent,
					Result*				aResult) override
				{
					BufferListWriter writer(m_bufferList);
					aItem->Write(&writer);

					m_pending.push_back(std::make_pair(aCompletionEvent, aResult));
				}

				size_t	
				Flush(
					ReplicationNetwork*	/*aReplicationNetwork*/) override
				{
					for(size_t i = 0; i < m_pending.size(); i++)
						m_pending[i].first->Signal();

					size_t count = m_pending.size();
					m_pending.clear();
					return count;
				}

				void
				Cancel() override
				{
					for (size_t i = 0; i < m_pending.size(); i++)
					{
						*(m_pending[i].second) = RESULT_CANCELED;
						m_pending[i].first->Signal();
					}

					m_pending.clear();
				}

				size_t
				GetPendingWriteCount() const override
				{
					return m_pending.size();
				}

				// Public data
				BufferList*											m_bufferList;
				std::vector<std::pair<CompletionEvent*, Result*>>	m_pending;
			};
		};

		//------------------------------------------------------------------------------

		MemoryHost::MemoryHost()
			: m_timeStamp(0)
		{
		}

		MemoryHost::~MemoryHost()
		{
			for (StoreMap::iterator i = m_storeMap.begin(); i != m_storeMap.end(); i++)
				delete i->second;

			for (WALMap::iterator i = m_walMap.begin(); i != m_walMap.end(); i++)
				delete i->second;
		}

		void					
		MemoryHost::DeleteAllFiles(
			uint32_t					aNodeId)
		{
			for(StoreMap::iterator i = m_storeMap.begin(); i != m_storeMap.end(); i++)
			{
				if(i->first.first == aNodeId || aNodeId == UINT32_MAX)
					m_storeMap.erase(i);
			}

			for (WALMap::iterator i = m_walMap.begin(); i != m_walMap.end(); i++)
			{
				if (i->first.first == aNodeId || aNodeId == UINT32_MAX)
					m_walMap.erase(i);
			}
		}

		//------------------------------------------------------------------------------

		IStats* 
		MemoryHost::GetStats()
		{
			static DummyStats dummyStats;
			return &dummyStats;
		}

		IConfigSource* 
		MemoryHost::GetConfigSource() 
		{
			return &m_defaultConfigSource;
		}

		Compression::IProvider* 
		MemoryHost::GetCompressionProvider() 
		{
			return NULL;
		}
		
		uint64_t				
		MemoryHost::GetTimeStamp() 
		{
			// Predictable current timestamp that always increments by 1 when queried
			return m_timeStamp++;
		}

		size_t					
		MemoryHost::GetAvailableDiskSpace() 
		{
			return SIZE_MAX;
		}

		void					
		MemoryHost::EnumerateFiles(
			uint32_t					aNodeId,
			std::vector<uint32_t>&		aOutWriteAheadLogIds,
			std::vector<uint32_t>&		aOutStoreIds) 
		{
			for (StoreMap::iterator i = m_storeMap.begin(); i != m_storeMap.end(); i++)
			{
				if(i->first.first == aNodeId)
					aOutStoreIds.push_back(i->first.second);
			}

			for (WALMap::iterator i = m_walMap.begin(); i != m_walMap.end(); i++)
			{
				if (i->first.first == aNodeId)
					aOutWriteAheadLogIds.push_back(i->first.second);
			}
		}
		
		void					
		MemoryHost::GetStoreInfo(
			uint32_t					aNodeId,
			std::vector<StoreInfo>&		aOut) 
		{
			for (StoreMap::iterator i = m_storeMap.begin(); i != m_storeMap.end(); i++)
			{
				if (i->first.first == aNodeId)
					aOut.push_back(IHost::StoreInfo{i->first.second, i->second->m_data.m_totalBytes});
			}
		}
		
		IFileStreamReader*		
		MemoryHost::ReadWALStream(
			uint32_t					aNodeId,
			uint32_t					aId,
			bool						/*aUseStreamingCompression*/,
			FileStatsContext*			/*aFileStatsContext*/)
		{
			WALMap::iterator i = m_walMap.find(std::make_pair(aNodeId, aId));
			JELLY_ASSERT(i != m_walMap.end());
			return i->second->Read();
		}
		
		IWALWriter*				
		MemoryHost::CreateWAL(
			uint32_t					aNodeId,
			uint32_t					aId,
			bool						/*aUseStreamingCompression*/,
			FileStatsContext*			/*aFileStatsContext*/)
		{
			WALMap::iterator i = m_walMap.find(std::make_pair(aNodeId, aId));
			JELLY_ASSERT(i == m_walMap.end());

			std::unique_ptr<WAL> wal = std::make_unique<WAL>();
			std::unique_ptr<IWALWriter> walWriter(wal->Write());

			m_walMap.insert(
				std::pair< 
					std::pair<uint32_t, uint32_t>, WAL*>(std::make_pair(aNodeId, aId), wal.release()));

			return walWriter.release();
		}
		
		void					
		MemoryHost::DeleteWAL(
			uint32_t					aNodeId,
			uint32_t					aId) 
		{
			WALMap::iterator i = m_walMap.find(std::make_pair(aNodeId, aId));
			JELLY_ASSERT(i != m_walMap.end());
			delete i->second;
			m_walMap.erase(i);
		}
		
		IFileStreamReader*		
		MemoryHost::ReadStoreStream(
			uint32_t					aNodeId,
			uint32_t					aId,
			FileStatsContext*			/*aFileStatsContext*/)
		{
			StoreMap::iterator i = m_storeMap.find(std::make_pair(aNodeId, aId));
			JELLY_ASSERT(i != m_storeMap.end());
			return i->second->ReadStream();
		}
		
		IStoreBlobReader*		
		MemoryHost::GetStoreBlobReader(
			uint32_t					aNodeId,
			uint32_t					aId,
			FileStatsContext*			/*aFileStatsContext*/)
		{
			StoreMap::iterator i = m_storeMap.find(std::make_pair(aNodeId, aId));
			JELLY_ASSERT(i != m_storeMap.end());
			return i->second->Read();
		}
				
		IStoreWriter*			
		MemoryHost::CreateStore(
			uint32_t					aNodeId,
			uint32_t					aId,
			FileStatsContext*			/*aFileStatsContext*/)
		{
			StoreMap::iterator i = m_storeMap.find(std::make_pair(aNodeId, aId));
			JELLY_ASSERT(i == m_storeMap.end());

			std::unique_ptr<Store> store = std::make_unique<Store>();
			std::unique_ptr<IStoreWriter> storeWriter(store->Write());

			m_storeMap.insert(
				std::pair< 
					std::pair<uint32_t, uint32_t>, Store*>(std::make_pair(aNodeId, aId), store.release()));

			return storeWriter.release();
		}
		
		void							
		MemoryHost::DeleteStore(
			uint32_t					aNodeId,
			uint32_t					aId) 
		{
			StoreMap::iterator i = m_storeMap.find(std::make_pair(aNodeId, aId));
			JELLY_ASSERT(i != m_storeMap.end());
			delete i->second;
			m_storeMap.erase(i);
		}

		File* 
		MemoryHost::CreateNodeLock(
			uint32_t					/*aNodeId*/) 
		{
			return NULL;
		}

	}

}