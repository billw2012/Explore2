#ifndef _RENDER_MEMORYMANAGER_HPP
#define _RENDER_MEMORYMANAGER_HPP

#include <stdexcept>
#include <sstream>
#include <list>
#include <map>
//#include <boost/shared_array.hpp>
#include <cmath>

namespace render
{

//struct BuddyMemoryManager
//{
//	enum Status { free, reserved };
//	struct Header
//	{
//		Status status : 1;
//		unsigned int k : sizeof(unsigned int)*8 - 1U;
//	};
//
//	struct Block : public Header
//	{
//		enum { size = 16 };
//		struct Links
//		{
//			Block* next;
//			Block* prev;
//		};
//
//		union
//		{
//			Links link;
//			unsigned int offs;
//			//unsigned char userPart[size - sizeof(Header)];
//		};
//	};
//
//private:
//	unsigned int m;
//	unsigned int numberOfBlocks;
//	std::vector<Block> pool;
//	Block* sentinal;
//
//	static void Unlink(Block& block)
//	{
//		block.link.next->link.prev = block.link.prev;
//		block.link.prev->link.next = block.link.next;
//	}
//
//	static void InsertAfter(Block& a, Block& b)
//	{
//		b.link.next = a.link.next;
//		a.link.next = &b;
//		b.link.prev = &a;
//	}
//
//	Block& Buddy(Block& block) const
//	{
//		// bit mask offset using size of block, bit is off then the block
//		// is even therefore its buddy is the next block, otherwise it is the
//		// previous block
//		if(block.offs & block.k == 0)
//		{
//			return *block.link.next;
//		}
//		return *block.link.prev;
//	}
//
//	static template < class FType >
//	FType log2ceil(FType val)
//	{
//		return std::log(val)/std::log(2);
//	}
//public:
//	BuddyMemoryManager(size_t bytes) 
//		: m(log2ceil(bytes)),
//		numberOfBlocks((1 << m) / sizeof(Block)),
//		pool(numberOfBlocks + m + 1),
//		sentinal(&pool[numberOfBlocks])
//	{
//		for(unsigned int i = 0; i <= m; ++i)
//		{
//			sentinal[i].link.next = &sentinal[i];
//			sentinal[i].link.prev = &sentinal[i];
//		}
//
//		Block& head = pool[0];
//		head.status = free;
//		head.k = m;
//		head.offs = 0;
//		InsertAfter(sentinal[m], head);
//	}
//
//	void* Acquire(size_t size)
//	{
//		unsigned int kPrime = std::log2(bytes + sizeof(Header));
//		unsigned int i = kPrime;
//
//		while(i <= m && sentinal[i].link.next == &sentinal[i])
//			++i;
//		if(i > m)
//			throw(std::bad_alloc("out of memory!"));
//
//		Block& block = *sentinal[i].link.prev;
//		Unlink(block);
//		while(block.k > kPrime)
//		{
//			block.k --;
//			Block& buddy = Buddy(block);
//			buddy.status = free;
//			buddy.k = block.k;
//			InsertAfter(sentinal[buddy.k], buddy);
//		}
//		block.status = reserved;
//		return block.userPart;
//	}
//
//	void Release(void* data)
//	{
//		Block& block = *reinterpret_cast<Block*>(reinterpret_cast<Header*>(arg) - 1U);
//		
//		if(&block < &pool[0] || &block >= pool + numberOfBlocks)
//			throw std::invalid_argument("invalid pointer");
//
//		block.status = free;
//		Block* ptr;
//		for(ptr = &block; ptr->k < m; ptr->k ++)
//		{
//			Block& block = Buddy(*ptr);
//			if(buddy.status == reserved || buddy.k != ptr->k)
//				break;
//			Unlink(buddy);
//			if(&buddy < ptr)
//				ptr = &buddy;
//		}
//
//		InsertAfter(sentinal[ptr->k], *ptr);
//	}
//};
template < class ValueType >
struct MemoryManager
{
	typedef ValueType value_type;
	typedef unsigned int size_type;
private:

	struct MemorySlot;
	//typedef std::auto_ptr< MemorySlot > MemorySlotPtr;
	typedef std::list< MemorySlot > MemorySlotDeque;
	typedef std::list< MemorySlot* > MemorySlotPtrDeque;
	typedef std::map< size_type, MemorySlotPtrDeque > MemorySlotDequeMap;
	typedef std::map< value_type, size_type > DataOffsetMap;

	struct MemorySlot
	{
	public:
		size_type offset;
		size_type size;
		bool used;
		value_type data;
		MemorySlot* buddy;
		MemorySlot* parent;
		//MemorySlotDeque::iterator parentItr;
		typename MemorySlotDeque::iterator itr;
		typename MemorySlotPtrDeque::iterator slotItr;

		MemorySlot(MemorySlot* buddy_, size_type offs, size_type size_, bool used_ = false, value_type dat = NULL)
			: itr(), slotItr(), parent(NULL), buddy(buddy_), offset(offs), size(size_), used(used_), data(dat)
		{}
	};

	MemorySlotDeque _allSlots;
	MemorySlotDequeMap _memorySlots;
	DataOffsetMap _currentData;
	size_type _minSlotSize;
	size_type _maxSlotSize;
	size_type _totalSize;

public:
	void initSlots(size_type minSize, size_type totalSize)
	{
		_memorySlots.clear();
		_currentData.clear();
		_allSlots.clear();
		_minSlotSize = static_cast<size_type>(std::pow(2.0f, (int)minSize));
		_totalSize = static_cast<size_type>(std::pow(2.0f, (int)totalSize));

		for(size_type i = _minSlotSize; i <= _totalSize; i*=2) 
		{
			_memorySlots[i] = MemorySlotPtrDeque();
		}
		//MemorySlotPtr root(new MemorySlot(NULL, 0, _totalSize));
		_allSlots.push_front(MemorySlot(NULL, 0, _totalSize));
		MemorySlot* root = &_allSlots.front();
		root->itr = _allSlots.begin();
		_memorySlots[_totalSize].push_front(root);
		root->slotItr = _memorySlots[_totalSize].begin();
// 		size_type offset = 0;
// 		for(int i = minSize; i <= maxSize; i++)
// 		{
// 			size_type chunkSize = static_cast<size_type>(std::pow(2.0f, i));
// 			_memorySlots[chunkSize] = MemorySlotDeque();
// 			for(size_type j = 0; j < slotCount * 2; j++)
// 			{
// 				_memorySlots[chunkSize].push_back(MemorySlot(offset));
// 				offset += chunkSize;
// 				_memorySlots[chunkSize].push_back(MemorySlot(offset));
// 				offset += chunkSize;
// 			}
// 		}
	}

	size_type getSize() const
	{
		return _totalSize;
	}

	bool isAlloced(const value_type data) const
	{
		return _currentData.find(data) != _currentData.end();
	}

	bool canAlloc(size_type requiredSize) const
	{
		return requiredSize <= _maxSlotSize;
	}

	void alloc(const value_type data, size_type requiredSize) throw(std::runtime_error)
	{
		// determine the next highest power of two that is allocatable
		size_type size = _minSlotSize;
		while(size < requiredSize) {
			size <<= 1;
			if(size > _totalSize) {
				std::ostringstream ost;
				ost << "Cannot allocate " << requiredSize << " bytes. Max available is " << _totalSize << " bytes.";
				throw std::runtime_error(ost.str());
			}
		}

		size_type locatedSize = size;
		// find a free slot of any size above that required
		while((_memorySlots[locatedSize].size() && _memorySlots[locatedSize].front()->used && locatedSize <= _totalSize) || 
				!_memorySlots[locatedSize].size()) {
			locatedSize *= 2;
		}

		// if we found a free slot
		if(locatedSize <= _totalSize) {
			// if it is the correct size
			//if(size == locatedSize)	{
			//	allocateBlock(locatedSize, data);
			//} // if it is too big then split it the required number of times
			//else {
			while(locatedSize > size) {
				// get empty slot from front of queue for current size
				// if we are just starting there will definitely be an empty slot
				// if we are part way through the loop then two empty slots should have been
				// placed here last loop
				splitBlock(locatedSize);
				locatedSize *= 0.5;
			}
			allocateBlock(locatedSize, data);
				//// allocate and return
				//MemorySlot* slot = _memorySlots[locatedSize]->front();
				//size_type offset = slot->offset;
				//slot->used = true;
				//slot->data = data;
				//// move the memory slot to the back of the deque and store the data in it
				//_memorySlots[locatedSize].push_back(slot);
				//_memorySlots[locatedSize].pop_front();
				//// move the slot to the front of all the slots
				//_allSlots->push_front(*slot->itr);
				//_allSlots->erase(slot->itr);
				//slot->itr = _allSlots.begin();
				//// map the new data
				//_currentData[data] = offset;
			//}
		}
		// if we didn't find a free slot then we must find the last most recently used
		// block of the required size and overwrite it
		else {
			size_type foundSize = 0;
			MemorySlotDeque::reverse_iterator sitr = _allSlots.rbegin();
			while(sitr != _allSlots.rend() && foundSize < size)	{
				// if our buddy is less recently used than we are
				//if(sitr->buddy->itr > sitr.base()) {
					MemorySlot* oldA = &(*sitr);
					MemorySlot* oldB = oldA->buddy;
					foundSize = oldA->size * 2;
					mergeBlocks(oldA, oldB);
				//}
			}

			while(foundSize > size) {
				splitBlock(foundSize);
				foundSize *= 0.5;
			}
			allocateBlock(foundSize, data);
		}


		//// quick(er?) way to find next highest pot:
		//// find highest set bit. it is pot if no bits are set below it, otherwise the pot is obtained by setting the 
		//// next highest bit, on its own.
		//// maybe not quicker loop will be the same length, in fact both methods are analogous

		//// find nearest higher pot size
		//int size = _minSlotSize;
		//while(size < requiredSize)
		//{
		//	size <<= 1;
		//	if(size > _maxSlotSize)
		//	{
		//		std::ostringstream ost;
		//		ost << "Cannot allocate " << requiredSize << " bytes. Max available is " << _maxSlotSize << " bytes.";
		//		throw std::runtime_error(ost.str());
		//	}
		//}

		//// if the first item in the slots for the required size is used then we could allocate a slot from the next highest chunk
		//// size, but for now just overwrite it
		//if(_memorySlots[size].front().data != NULL)
		//{
		//	DataOffsetMap::iterator it = _currentData.find(_memorySlots[size].front().data);
		//	if(it != _currentData.end())
		//		_currentData.erase(it);
		//}

		//size_type offset = _memorySlots[size].front().offset;
		//// move the memory slot to the back of the deque and store the data in it
		//_memorySlots[size].push_back(MemorySlot(offset, data));
		//_memorySlots[size].pop_front();
		//// map the new data
		//_currentData[data] = offset;
	}

	void mergeBlocks( MemorySlot* oldA, MemorySlot* oldB )
	{
		assert(oldA->parent == oldB->parent);

		_memorySlots[oldA->size].erase(oldA->slotItr);
		_memorySlots[oldB->size].erase(oldB->slotItr);

		// merge blocks
		MemorySlot* slot = oldA->parent;
		slot->used = false;
		_memorySlots[oldA->size*2].push_front(slot);
		slot->slotItr = _memorySlots[oldA->size*2].begin();

		_allSlots.erase(oldA->itr);
		_allSlots.erase(oldB->itr);
	}

	void allocateBlock( size_type locatedSize, const value_type data )
	{
		// allocate and return
		MemorySlot* slot = _memorySlots[locatedSize].front();
		size_type offset = slot->offset;
		slot->used = true;
		slot->data = data;
		// move the memory slot to the back of the deque and store the data in it
		_memorySlots[locatedSize].push_back(slot);
		_memorySlots[locatedSize].pop_front();
		slot->slotItr = _memorySlots[locatedSize].end() - 1;
		// move the slot to the front of all the slots
		_allSlots.push_front(*slot->itr);
		_allSlots.erase(slot->itr);
		slot->itr = _allSlots.begin();
		// map the new data
		_currentData[data] = offset;
	}

	void splitBlock( size_type locatedSize )
	{
		MemorySlot* slot = _memorySlots[locatedSize].front();
		// remove it from the queue
		_memorySlots[locatedSize].pop_front();

		size_type splitSize = locatedSize*0.5;
		// create two new slots from it
		//MemorySlotPtr newA(new MemorySlot(NULL, slot->offset, splitSize));
		//MemorySlotPtr newB(new MemorySlot(newA.get(), slot->offset + splitSize, splitSize));

		// add the new slots to the full list at the end so they are used first
		_allSlots.push_back(MemorySlot(NULL, slot->offset, splitSize));
		MemorySlot* newA = &_allSlots.front();
		newA->itr = _allSlots.end() - 1;
		_allSlots.push_back(MemorySlot(newA, slot->offset + splitSize, splitSize));
		MemorySlot* newB = &_allSlots.front();
		newB->itr = _allSlots.end() - 1;
		newA->buddy = newB;

		newA->parent = slot;
		newB->parent = slot;

		// add the new slots to their size queue
		_memorySlots[splitSize].push_front(newB);
		newB->slotItr = _memorySlots[splitSize].begin();
		_memorySlots[splitSize].push_front(newA);
		newA->slotItr = _memorySlots[splitSize].begin();
	}

	size_type getOffset(const value_type data) const
	{
		DataOffsetMap::const_iterator it = _currentData.find(data);
		assert(it != _currentData.end());
		return it->second;
	}
	// need some sort of flag to say whether the data was already allocated
	//size_type request(DataType data, size_type requiredSize) throw(std::runtime_error);

	size_type getMinSlotSize() const
	{
		return _minSlotSize;
	}

	size_type getMaxSlotSize() const
	{
		return _maxSlotSize;
	}

	const MemorySlotDequeMap& getMemoryMap() const
	{
		return _memorySlots;
	}
};

}

#endif // _RENDER_MEMORYMANAGER_HPP