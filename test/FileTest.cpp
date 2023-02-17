#include <jelly/API.h>

namespace jelly
{

	namespace Test
	{

		namespace
		{
			
			struct FileStats
				: public IStats
			{
				enum Id : uint32_t
				{
					ID_READ,
					ID_WRITE,

					NUM_IDS
				};

				FileStats()
				{
					memset(m_counters, 0, sizeof(m_counters));
				}

				// IStats implementation
				void				
				Emit_UInt64(
					uint32_t							aId,
					uint64_t							aValue,
					const std::optional<Stat::Type>&	aExpectedType) override
				{
					JELLY_ASSERT(aId < (uint32_t)NUM_IDS);
					JELLY_ASSERT(aExpectedType == Stat::TYPE_COUNTER);
					JELLY_UNUSED(aExpectedType);
					m_counters[aId] += aValue;
				}

				// Public data
				uint64_t	m_counters[NUM_IDS];
			};

		}

		namespace FileTest
		{

			void		
			Run()
			{
				FileStats fileStats;
				FileStatsContext context(&fileStats);
				context.m_idRead = FileStats::ID_READ;
				context.m_idWrite = FileStats::ID_WRITE;

				static const char* TEST_DATA = "TestHelloWorldTest!";
				size_t testDataSize = strlen(TEST_DATA);
				JELLY_UNUSED(testDataSize);

				// Try to read stream file that's not there
				{
					File f(&context, "testfile2.tmp", File::MODE_READ_STREAM, FileHeader());
					JELLY_ASSERT(!f.IsValid());
				}

				// Try to read random-access file that's not there
				{
					File f(&context, "testfile2.tmp", File::MODE_READ_RANDOM, FileHeader());
					JELLY_ASSERT(!f.IsValid());
				}

				// Create a file
				{
					File f(&context, "testfile.tmp", File::MODE_WRITE_STREAM, FileHeader());
					JELLY_ASSERT(f.IsValid());
					JELLY_ASSERT(f.Write(TEST_DATA, testDataSize) == testDataSize);
					JELLY_ASSERT(f.GetSize() == testDataSize + sizeof(FileHeader));
					JELLY_ASSERT(f.Flush() == testDataSize + sizeof(FileHeader));
					JELLY_ASSERT(fileStats.m_counters[FileStats::ID_READ] == 0);
					JELLY_ASSERT(fileStats.m_counters[FileStats::ID_WRITE] == testDataSize);
				}

				// Read file as a stream
				{
					File f(&context, "testfile.tmp", File::MODE_READ_STREAM, FileHeader());
					JELLY_ASSERT(f.IsValid());
					JELLY_ASSERT(f.GetSize() == testDataSize + sizeof(FileHeader));
					char buffer[256];
					JELLY_UNUSED(buffer);
					JELLY_ASSERT(sizeof(buffer) >= testDataSize);
					JELLY_ASSERT(f.Read(buffer, testDataSize) == testDataSize);
					JELLY_ASSERT(memcmp(buffer, TEST_DATA, testDataSize) == 0);
					JELLY_ASSERT(fileStats.m_counters[FileStats::ID_READ] == testDataSize);
					JELLY_ASSERT(fileStats.m_counters[FileStats::ID_WRITE] == testDataSize);
				}

				// Read file with random access
				{
					File f(&context, "testfile.tmp", File::MODE_READ_RANDOM, FileHeader());
					JELLY_ASSERT(f.IsValid());
					char buffer[11];
					buffer[10] = '\0';
					f.ReadAtOffset(4 + sizeof(FileHeader), buffer, 10);
					JELLY_ASSERT(strcmp(buffer, "HelloWorld") == 0);
					JELLY_ASSERT(fileStats.m_counters[FileStats::ID_READ] == testDataSize + 10);
					JELLY_ASSERT(fileStats.m_counters[FileStats::ID_WRITE] == testDataSize);
				}

				// Open a file as a stream and while still open, also open it as random access
				{
					File fStream(&context, "testfile.tmp", File::MODE_READ_STREAM, FileHeader());
					JELLY_ASSERT(fStream.IsValid());

					{
						File fRandom(&context, "testfile.tmp", File::MODE_READ_RANDOM, FileHeader());
						JELLY_ASSERT(fRandom.IsValid());
					}
				}

				// Same thing, but other way around
				{
					File fRandom(&context, "testfile.tmp", File::MODE_READ_RANDOM, FileHeader());
					JELLY_ASSERT(fRandom.IsValid());

					{
						File fStream(&context, "testfile.tmp", File::MODE_READ_STREAM, FileHeader());
						JELLY_ASSERT(fStream.IsValid());
					}
				}

				// Delete the file (this tests that all handles are released)
				JELLY_ASSERT(std::filesystem::remove("testfile.tmp"));
			}

		}

	}

}