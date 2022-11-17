//#include <iostream>
//
//#include <vector>
//
//#include <chrono>
//
//#define NOMINMAX
//#include <Windows.h>
//
//#include "Page.h"
//
//class Entity
//{
//public:
//	size_t m_value = 0;
//	int m_intArr[32] = {};
//
//public:
//	void IncrementValue()
//	{
//		m_value++;
//	};
//
//	inline auto& Value()
//	{
//		return m_value;
//	};
//
//};
//
//constexpr static size_t NUM_LOOPS = 1'000'00;
//
//template <typename C>
//size_t Timing(C call)
//{
//	auto start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
//	//=========================================================================================
//
//	call();
//
//	//=========================================================================================
//	auto end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
//	return end - start;
//}
//
//int main()
//{
//	auto lo = std::wcout.imbue(std::locale("en_US.utf8"));
//	SetConsoleCP(65001);
//	SetConsoleOutputCP(CP_UTF8);
//
//	memory::Page page;
//
//	int* arr1 = (int*)page.Allocate(16 * sizeof(int));
//
//	for (int i = 0; i < 16; i++)
//	{
//		arr1[i] = i;
//	}
//
//	int* arr2 = (int*)page.Allocate(16 * sizeof(int));
//
//	float* fArr1 = (float*)page.Allocate(16 * sizeof(float));
//
//	for (int i = 0; i < 16; i++)
//	{
//		fArr1[i] = i / 10.0f;
//	}
//
//	page.Free(arr2);
//
//	page.MemoryDefragment(-1, 
//		[&](byte* addr, size_t size, size_t defragOffset) 
//		{
//			fArr1 = (float*)((byte*)fArr1 - defragOffset);
//		}
//	);
//
//	for (int i = 0; i < 16; i++)
//	{
//		std::cout << fArr1[i] << ", ";
//	}
//
//	page.Free(fArr1);
//	page.Free(arr1);
//
//	/*std::cout << "Accessing via Ptr<T>: " << Timing([&]()
//		{
//			static Entity* entities[NUM_LOOPS] = {};
//			static int* arrs[NUM_LOOPS] = {};
//
//			for (size_t i = 0; i < NUM_LOOPS; i++)
//			{
//				auto entity = (Entity*)page.Allocate(sizeof(Entity));
//				new (entity) Entity();
//				entities[i] = entity;
//			}
//
//			for (size_t i = 0; i < NUM_LOOPS; i++)
//			{
//				if (i % 2)
//				{
//					page.Free(entities[i]);
//				}
//			}
//
//			for (size_t i = 0; i < NUM_LOOPS; i++)
//			{
//				arrs[i] = (int*)page.Allocate(64 * sizeof(int));
//			}
//
//			for (size_t i = 0; i < NUM_LOOPS; i++)
//			{
//				if (i % 2 == 0)
//				{
//					page.Free(entities[i]);
//					page.Free(arrs[i]);
//				}
//			}
//
//			for (size_t i = 0; i < NUM_LOOPS; i++)
//			{
//				if (i % 2)
//				{
//					page.Free(arrs[i]);
//				}
//			}
//		}
//	) << "ms\n";
//
//	std::cout << "Accessing via Raw ptr: " << Timing([]()
//		{
//			static Entity* entities[NUM_LOOPS] = {};
//			static int* arrs[NUM_LOOPS] = {};
//
//			for (size_t i = 0; i < NUM_LOOPS; i++)
//			{
//				auto entity = new Entity();
//
//				entity->IncrementValue();
//
//				entities[i] = entity;
//				delete entity;
//			}
//
//			for (size_t i = 0; i < NUM_LOOPS; i++)
//			{
//				if (i % 2)
//					delete entities[i];
//			}
//
//			for (size_t i = 0; i < NUM_LOOPS; i++)
//			{
//				arrs[i] = new int[64];
//			}
//
//			for (size_t i = 0; i < NUM_LOOPS; i++)
//			{
//				if (i % 2 == 0)
//				{
//					delete entities[i];
//					delete[] arrs[i];
//				}
//			}
//
//			for (size_t i = 0; i < NUM_LOOPS; i++)
//			{
//				if (i % 2)
//				{
//					delete[] arrs[i];
//				}
//			}
//		}
//	) << "ms\n";*/
//	
//
//	return 0;
//}