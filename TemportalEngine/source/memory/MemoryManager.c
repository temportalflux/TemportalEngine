#include "memory/MemoryManager.h"

#include "stdio.h"
#include "math/compare.h"
#include <string.h>  // for memset
#include <cassert>

typedef struct a3_mem_NodeHeader	a3_mem_NodeHeader;
typedef struct a3_mem_ChunkHeader	a3_mem_ChunkHeader;

typedef void(*RotationFunction)(a3_mem_NodeHeader *const, a3_mem_NodeHeader**);
typedef a3_mem_NodeHeader**(*GetChildBranch)(a3_mem_NodeHeader*);

// Structures ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
* The header of each fragment of managed memory.
* Total Size: x86: 25 bytes, x64: 56 bytes
*/
struct a3_mem_NodeHeader
{

	// General: 8 bytes

	// The size of the memory that will be allocated for the node
	// integer (x86: 4 bytes, x64: 8 bytes)
	uSize size;

	// Used to check if the data here is allocated or not
	// 0 = free (false)
	// 1 = allocated (true)
	// 32-bit integer (4 bytes)
	ui8 isAllocated;

	// ~~~ Start: Doubly Linked List in linear memory (x86: 4 bytes, x64: 8 bytes)

	// The "previous" node in a linked list
	// ptr (x86: 4 bytes, x64: 8 bytes)
	a3_mem_NodeHeader *prev;

	// ~~~~~ END: Doubly Linked List

	// ~~~ Start: AVL tree of free memory nodes (x86: 13 bytes, x64: 25 bytes)

	// The parent node (implies that this node is either a left or right of parent).
	// ptr (x86: 4 bytes, x64: 8 bytes)
	a3_mem_NodeHeader *parent;

	// The left node in the binary tree
	// ptr (x86: 4 bytes, x64: 8 bytes)
	a3_mem_NodeHeader *left;

	// The right node in the binary tree
	// ptr (x86: 4 bytes, x64: 8 bytes)
	a3_mem_NodeHeader *right;

	// Height of node in the binary tree
	// using a char from 0 to 255 since no tree's height should be bigger then 255
	// 1 byte
	char height;

	// ~~~~~ END: AVL tree

};

/**
* The initialized header to keep track of the entire block of managed memory.
* Total Size: x86: 8 bytes, x64: 16 bytes
*/
struct a3_mem_ChunkHeader
{

	// The total size of the memory being managed (including all headers)
	// integer (x86: 4 bytes, x64: 8 bytes)
	uSize chunkSize;

	// The root of the AVL tree of free headers
	// ptr (x86: 4 bytes, x64: 8 bytes)
	a3_mem_NodeHeader* freeNodeRoot;

};

// Linear Memory: Getters ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
* Purpose: Gets the first node in linear memory.
* Pre: The memory must have been initialized by a3_mem_init.
* Post: Returns the node header immediately after the chunkHeader.
* Performance: O(1)
*/
a3_mem_NodeHeader* a3_mem_getFirstNode(a3_mem_ChunkHeader* chunkHeader);

/**
* INTERNAL USE: Linear Memory
* Purpose: Get the next node in linear memory
* Pre: current - An initialized node header.
* Post: Returns the next node in linear memory. Null if current is actually the last node in the block.
* Performance: O(1)
*/
a3_mem_NodeHeader* a3_mem_getNextNode(a3_mem_ChunkHeader* chunkHeader, a3_mem_NodeHeader* current);

/**
* INTERNAL USE: Linear Memory
* Purpose: Retrieve the address for the end of the managed slab of memory.
* Pre: a3_mem_init has been called on the block of memory (initializing the chunk header).
* Post: Returns the memory address immediately after the managed chunk of memory.
* Performance: O(1)
*/
char* a3_mem_getChunkEnd(a3_mem_ChunkHeader* chunkHeader);

/**
* INTERNAL USE: Linear Memory
* Purpose: Retrieve the header for a given chunk of memory
* Pre: Some memory address returned by a3_mem_alloc
* Post: Returns the header for the given memory
* Performance: O(1)
*/
a3_mem_NodeHeader* a3_mem_getNodeForMemoryChunk(void* memoryAddress);

/**
* INTERNAL USE: Linear Memory
* Purpose: Get the next node where current starts at a given memory location in the block.
* Pre: If current is a struct passed by reference, location is the location where the node is in the block,
*    otherwise location and current are the same pointer (use a3_mem_getNextNode(ChunkHeader, NodeHeader).
* Post: Returns the next node in linear memory.
* Performance: O(1)
*/
a3_mem_NodeHeader* a3_mem_getNextNodeAtLocation(a3_mem_ChunkHeader* chunkHeader, char* location, a3_mem_NodeHeader* current);

// AVL Tree: Getters ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

a3_mem_NodeHeader** a3_mem_getReferenceToChildLeft(a3_mem_NodeHeader* node);

a3_mem_NodeHeader** a3_mem_getReferenceToChildRight(a3_mem_NodeHeader* node);

/**
* Purpose: Get a reference to the parent node's pointer to a given node.
* Pre: node has a parent.
* Post: Returns a reference to the pointer in the parent for this node (node->parent->left/right).
* Performance: O(1)
*/
a3_mem_NodeHeader** a3_mem_getParentReference(a3_mem_NodeHeader* node);

/**
* INTERNAL USE: AVL Tree of Free Nodes
* Purpose: Find the smallest node in a subtree.
* Pre: A node that is a part of the AVL tree of free nodes.
* Post: Traverses the subtree to find the far left leaf node. Can return the node itself.
* Performance: O(n) at worst where n is the height of the node in just the left column.
*/
a3_mem_NodeHeader* a3_mem_getSmallestNode(a3_mem_NodeHeader* node);

/**
* INTERNAL USE: AVL Tree of Free Nodes
* Purpose: Determine the height of a given node, even if it does not exist.
* Pre: Pointer to some 'node'. Can be nil.
* Post: Returns node->height if node is non-nil, -1 if node is nil.
* Performance: O(1)
*/
char a3_mem_getHeightSafe(a3_mem_NodeHeader const * const node);

/**
* INTERNAL USE: AVL Tree of Free Nodes
* Purpose: Calculates the balance factor for a given node.
*    Values whose absolute value > 1 indicate that the subtree needs to be rebalanced.
* Pre: The height of the provided node and any children (node's left and right variables)
*    has been calculated using a3_mem_calculateHeight.
* Post: Returns the balance factor of the given node.
* Performance: O(1)
*/
char a3_mem_getBalanceFactor(a3_mem_NodeHeader const * const node);

/**
* INTERNAL USE: AVL Tree of Free Nodes
* Purpose: Calculate the height for some given node.
* Pre: Some node header
* Post: Returns the max height of the tree children of the node + 1.
*    max(height(left), height(right)) + 1
* Performance: O(1)
*/
char a3_mem_calculateHeight(a3_mem_NodeHeader const * const node);

/**
* INTERNAL USE: AVL Tree of Free Nodes
* Purpose: Calculate and set the height of a node ancestry.
* Pre: node is in the AVL tree of free nodes.
* Post: Updates the heights of all nodes in the parent ancestry.
* Performance: O(n) where n is the distance of the node from the root node.
*/
void a3_mem_calculateHeightOfAncestry(a3_mem_NodeHeader *node);

// AVL Tree: Management ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
* INTERNAL USE: AVL Tree of Free Nodes
* Purpose: Adds a node to the AVL tree of free nodes
* Pre: node.isAllocated is 0. The node is not in the AVL tree.
* Post: The node will be added to the AVL tree, thereby populating AVL vars: parent, left, right, and height. Will NOT balance the AVL tree.
*/
void a3_mem_addNodeToFreeTree(a3_mem_ChunkHeader* chunkHeader, a3_mem_NodeHeader* node);

/**
* Purpose: Attach a node to a parent in the tree.
* Pre: nope nothing :)
* Post: The node will have a parent and the parent will be linked to the node.
*/
void a3_mem_attachNodeToTree(a3_mem_ChunkHeader* chunkHeader, a3_mem_NodeHeader* node);

/**
* Purpose: Search for the appropriate location in the AVL tree to put the node.
* Pre: nope nothing :P
* Post: Will return the pointer to the parent node that the node should be attached to.
*/
a3_mem_NodeHeader* a3_mem_findParentForNodeInsertion(a3_mem_NodeHeader* root, a3_mem_NodeHeader* node);

/**
* INTERNAL USE: AVL Tree of Free Nodes
* Purpose: Removes a node from the AVL tree
* Pre: node.isAllocated is 1. The node MUST be in the AVL tree (a3_mem_addNodeToFreeTree was called with this node).
* Post: Uninitializes node variables parent, left, right, and height, and remaps other nodes in the tree without this node.
*/
void a3_mem_removeNodeFromFreeTree(a3_mem_ChunkHeader* chunkHeader, a3_mem_NodeHeader* node);

/**
* INTERNAL USE: AVL Tree of Free Nodes
* Purpose: Merge the the second node into the first node
* Pre: Both nodes must have isAllocated marked as 1 and must be removed from the AVL tree.
*    Pass these nodes by reference.
* Post: Make sure the first node is re-added to the AVL tree
*/
void a3_mem_mergeFreeNodes(a3_mem_ChunkHeader* chunkHeader, a3_mem_NodeHeader* first, a3_mem_NodeHeader* second);

/**
* INTERNAL USE: AVL Tree of Free Nodes
* Purpose: Balance the free node AVL tree
* Pre: Needs the chunk header and assumes operations were performed on the free node tree.
* Post: Balances the free node tree, rotating where necessary
*/
void a3_mem_connectFreeNodeToParent(a3_mem_ChunkHeader* chunkHeader, a3_mem_NodeHeader* node, a3_mem_NodeHeader* childNode);

// AVL Tree: Balacing/Rotations ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
* Purpose: Will rotate the tree at this node if the subtree is not balanced.
* Pre: node must be a non-allocated node (isAllocated == 0).
* Post: The subtree at node will have an absolute balance factor < 2 (it is balanced).
*/
void a3_mem_balanceSubtree(a3_mem_ChunkHeader*const chunkHeader, a3_mem_NodeHeader *const node);

/**
* Purpose: Rotates the tree at the given node to the left.
* Pre: node is a free node, and root is a reference to the pointer of the root of the AVL free tree
* Post: Nodes will be organized as follow, if node is 'a'
*    a
*     \             b
*      b    =>    /   \
*       \        a     c
*        c
*/
void a3_mem_rotateLeft(a3_mem_NodeHeader *const node, a3_mem_NodeHeader** root);

/**
* Purpose: Rotates the tree at the given node to the right.
* Pre: node is a free node, and root is a reference to the pointer of the root of the AVL free tree
* Post: Nodes will be organized as follow, if node is 'a'
*        a
*       /           b
*      b    =>    /   \
*     /          a     c
*    c
*/
void a3_mem_rotateRight(a3_mem_NodeHeader *const node, a3_mem_NodeHeader** root);

void a3_mem_rotate(a3_mem_NodeHeader *const node, a3_mem_NodeHeader** root, GetChildBranch getBranchA, GetChildBranch getBranchB);

/**
* Purpose: Rotates the tree at the given node to left, and then to the right.
* Pre: node is a free node, and root is a reference to the pointer of the root of the AVL free tree
* Post: Nodes will be organized as follow, if node is 'a'
*        a          c
*       /          / \
*      b    =>    b   a
*     / \        /
*    d   c      d
*/
void a3_mem_rotateLeftRight(a3_mem_NodeHeader *const node, a3_mem_NodeHeader** root);

/**
* Purpose: Rotates the tree at the given node to right, and then to the left.
* Pre: node is a free node, and root is a reference to the pointer of the root of the AVL free tree
* Post: Nodes will be organized as follow, if node is 'a'
*    a              c
*     \            / \
*      b    =>    a   b
*     / \              \
*    c   d              d
*/
void a3_mem_rotateRightLeft(a3_mem_NodeHeader *const node, a3_mem_NodeHeader** root);

RotationFunction a3_mem_pickRotation(a3_mem_NodeHeader* secondaryNode, RotationFunction negative, RotationFunction positive);

// AVL Tree: Search ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
* INTERNAL USE: AVL Tree of Free Nodes
* Purpose: Find a free node with the smallest size from all nodes whose size is greater than or equal to the total requested size.
* Pre: totalSize is the size of the requestedMemory + the size of the header
* Post: Returns the memory address of the "best" node, else returns null
*/
a3_mem_NodeHeader* a3_mem_findSmallestNodeWithMinimumSize(a3_mem_ChunkHeader* chunkHeader, uSize minimumSize);

// Public functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

uSize a3_mem_manager_totalChunkSize(uSize nodeCount)
{
	return sizeof(a3_mem_ChunkHeader) + (nodeCount * sizeof(a3_mem_NodeHeader));
}

uSize a3_mem_manager_indexOfPtr(void* block, void* ptr, uSize uniformSize)
{
	// Get the node for a ptr by casting a ptr to a node ptr (changing its interpreted size & properties)
	// and shifting it back by one (aligning it to the actual ptr for the node).
	// Then interpret as an unknown type
	void* nodePtr = ((a3_mem_NodeHeader*)ptr) - 1;
	uSize totalSizePerAllocation = sizeof(a3_mem_NodeHeader) + uniformSize;
	void* firstNodePtr = a3_mem_getFirstNode((a3_mem_ChunkHeader*)block);
	return ((uSize)nodePtr - (uSize)firstNodePtr) / totalSizePerAllocation;
}

ui8 a3_mem_manager_init(void* block, uSize size)
{

	// Initialize the chunk header

	// Determine the size of the chunk header
	uSize chunkHeaderSize = sizeof(a3_mem_ChunkHeader);

	assert(chunkHeaderSize < size);

	// Cast the full memory block to an a3_ChunkHeader to properly edit bytes
	a3_mem_ChunkHeader* chunkHeader = (a3_mem_ChunkHeader*)block;

	// Assign the size of managed memory to the total block size without the size of the chunk header
	chunkHeader->chunkSize = size - chunkHeaderSize;

	// Make a free node

	// Format the header of the free block to have a header
	a3_mem_NodeHeader* nodeHeader = a3_mem_getFirstNode(chunkHeader);
	
	// Mark the header as not having allocated memory
	nodeHeader->isAllocated = 0;
	
	// There are no nodes to the left or right, so nullptr
	nodeHeader->prev = 0;
	nodeHeader->parent = 0;
	nodeHeader->left = 0;
	nodeHeader->right = 0;
	nodeHeader->height = 0;
	// The size of the free node's memory chunk is the total remaining size without the size of the node header
	nodeHeader->size = chunkHeader->chunkSize - sizeof(a3_mem_NodeHeader);

	// Save the address of the root free node
	chunkHeader->freeNodeRoot = nodeHeader;

	return 1;
}

ui8 a3_mem_manager_alloc(void* block, uSize size, void** requestedMemory)
{
	*requestedMemory = 0;

	if (size <= 0)
	{
		printf("Cannot allocate less than or equal to 0 bytes. Bad monkey, fix your code.\n");
		return 0;
	}

	// Get the chunk header
	a3_mem_ChunkHeader* chunkHeader = (a3_mem_ChunkHeader*)block;

	// Determine the size of the node header
	uSize sizeOfNodeHeader = sizeof(a3_mem_NodeHeader);

	// And the total size that needs to be allocated
	uSize sizeToAlloc = size + sizeOfNodeHeader;

	// Find the best free node to put the allocation in
	char* freeNodeLocation = (char*)a3_mem_findSmallestNodeWithMinimumSize(chunkHeader, size);

	// If no node was returned, we cannot allocate
	if (freeNodeLocation <= 0)
	{
		printf("Could not allocate %u bytes.\n", (unsigned int)size);
		return 0;
	}

	// Get the free node header at the provided location
	a3_mem_NodeHeader* freeNode = (a3_mem_NodeHeader*)freeNodeLocation;
	a3_mem_NodeHeader formerFreeNode = *freeNode;

	// Move the free node to a new location in memory (specifically offseted by the size of the new header and the size requested)
	{
		// Remove the free node from the AVL tree
		a3_mem_removeNodeFromFreeTree(chunkHeader, freeNode);
		freeNode = 0;

		if (formerFreeNode.size - size > 0)
		{
			// Slot off a new portion for the free header to occupy
			a3_mem_NodeHeader* nextNodeHeader = (a3_mem_NodeHeader*)(freeNodeLocation + sizeToAlloc);

			nextNodeHeader->size = formerFreeNode.size - sizeToAlloc;
			nextNodeHeader->isAllocated = 0;

			nextNodeHeader->prev = formerFreeNode.prev;

			nextNodeHeader->parent = 0;
			nextNodeHeader->left = 0;
			nextNodeHeader->right = 0;
			nextNodeHeader->height = 0;

			freeNode = nextNodeHeader;

			// NOTE: Quirk in this system; if the sizeToAlloc (header & requested size) == size given to the free node
			// then we can't do anything with the space of the header (can't exactly give it to the previous if the next space opens up).
			// Therefore, it is possible for these Zeroed-Free Nodes to exist in linear memory, but not in the AVL tree of Free Nodes 
			if (nextNodeHeader->size > 0)
			{
				// Add the freeNodeHeader to the AVL tree
				a3_mem_addNodeToFreeTree(chunkHeader, freeNode);
			}
		}
	}
	
	// Initialize the header for the allocated memory
	a3_mem_NodeHeader* allocatedNode = (a3_mem_NodeHeader*)freeNodeLocation;

	// Mark down the size of the block of memory the header is for
	allocatedNode->size = size;
	// Yes, the header is for allocated memory
	allocatedNode->isAllocated = 1;
	
	// Allocated node is directly before the now-moved free node in linear memory
	// Redirect the allocate prev to be the free node's prev
	allocatedNode->prev = formerFreeNode.prev;

	// and the free node's prev to be the allocated node
	if (freeNode != 0)
	{
		freeNode->prev = allocatedNode;
	}

	a3_mem_NodeHeader* next = a3_mem_getNextNodeAtLocation(chunkHeader, freeNodeLocation, &formerFreeNode);
	if (next != 0)
		next->prev = freeNode != 0 ? freeNode : allocatedNode;

	// These will never be used while the node is marked as isAllocated
	allocatedNode->parent = 0;
	allocatedNode->left = 0;
	allocatedNode->right = 0;
	allocatedNode->height = 0;
	
	char* bodyAddress = (char*)allocatedNode + sizeOfNodeHeader;
	*requestedMemory = bodyAddress;
	// TODO: pass as parameter if the block should be preformatted
	memset(bodyAddress, 0, allocatedNode->size);

	return 1;
}

void a3_mem_manager_deallocUniform(void* block, uSize idx, uSize uniformSize)
{
	uSize totalSizePerAllocation = sizeof(a3_mem_NodeHeader) + uniformSize;
	void* firstNodePtr = a3_mem_getFirstNode((a3_mem_ChunkHeader*)block);
	uSize nodeAtIdx = (uSize)firstNodePtr + (idx * totalSizePerAllocation);
	a3_mem_manager_deallocAndClear(block, (void*)(nodeAtIdx + sizeof(a3_mem_NodeHeader)), 0);
}

void a3_mem_manager_dealloc(void* block, void* allocatedLocation)
{
	a3_mem_manager_deallocAndClear(block, allocatedLocation, 0);
}

void a3_mem_manager_deallocAndClear(void* block, void* allocatedLocation, char clear)
{
	// Get the node for a give chunk of memory
	a3_mem_NodeHeader* node = a3_mem_getNodeForMemoryChunk(allocatedLocation);

	node->isAllocated = 0;

	// Add the new free node to the AVL tree
	a3_mem_addNodeToFreeTree((a3_mem_ChunkHeader*)block, node);

	// Attempt to merge node with a free node before it in linear memory
	a3_mem_NodeHeader* previousNode = node->prev;
	if (previousNode != 0 && previousNode->isAllocated == 0)
	{
		// Merge the nodes
		// This will modify: linear memory list, AVL tree, node size
		a3_mem_mergeFreeNodes((a3_mem_ChunkHeader*)block, previousNode, node);

		// The node has been merged into previous node, so point to the previous,
		// not somewhere in the middle of the memory chunk of the previous
		node = previousNode;
	}

	// Attempt to merge node with a free node after it in linear memory
	a3_mem_NodeHeader* nextNode = a3_mem_getNextNode((a3_mem_ChunkHeader*)block, node);
	if (nextNode != 0 && nextNode->isAllocated == 0)
	{
		// Merge the nodes
		// This will modify: linear memory list, AVL tree, node size
		a3_mem_mergeFreeNodes((a3_mem_ChunkHeader*)block, node, nextNode);
		// the next node has been merged into node, node is still the accurate header
	}

	if (clear != 0) memset((char*)node, 0, node->size + sizeof(a3_mem_NodeHeader));

}

void* a3_mem_manager_nodeAtUniformIndex(void* block, uSize uniformSize, uSize idx)
{
	a3_mem_ChunkHeader* chunkHeader = (a3_mem_ChunkHeader*)block;
	uSize node = (uSize)a3_mem_getFirstNode(chunkHeader);
	uSize totalSizePerAllocation = sizeof(a3_mem_NodeHeader) + uniformSize;
	return (a3_mem_NodeHeader*)(node + (idx * totalSizePerAllocation));
}

void* a3_mem_manager_atUniformIndex(void* block, uSize uniformSize, uSize idx, ui8 *isAllocated)
{
	a3_mem_NodeHeader* nodeAtIdx = a3_mem_manager_nodeAtUniformIndex(block, uniformSize, idx);
	*isAllocated = nodeAtIdx->isAllocated;
	return (void*)(nodeAtIdx + 1);
}

// Linear Memory: Getters ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

char* a3_mem_getChunkEnd(a3_mem_ChunkHeader* chunkHeader)
{
	return (char*)chunkHeader + sizeof(a3_mem_ChunkHeader) + chunkHeader->chunkSize;
}

a3_mem_NodeHeader* a3_mem_getNodeForMemoryChunk(void* memoryAddress)
{
	return (a3_mem_NodeHeader*)memoryAddress - 1;
}

a3_mem_NodeHeader* a3_mem_getFirstNode(a3_mem_ChunkHeader* chunkHeader)
{
	return (a3_mem_NodeHeader*)(chunkHeader + 1);
}

a3_mem_NodeHeader* a3_mem_getNextNode(a3_mem_ChunkHeader* chunkHeader, a3_mem_NodeHeader* current)
{
	return a3_mem_getNextNodeAtLocation(chunkHeader, (char*)current, current);
}

a3_mem_NodeHeader* a3_mem_getNextNodeAtLocation(a3_mem_ChunkHeader* chunkHeader, char* location, a3_mem_NodeHeader* current)
{
	char* nextLocation = location + sizeof(a3_mem_NodeHeader) + current->size;
	if (nextLocation == a3_mem_getChunkEnd(chunkHeader)) return 0;
	else return (a3_mem_NodeHeader*)(nextLocation);
}

// AVL Tree: Getters ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

a3_mem_NodeHeader** a3_mem_getReferenceToChildLeft(a3_mem_NodeHeader* node)
{
	return &(node->left);
}

a3_mem_NodeHeader** a3_mem_getReferenceToChildRight(a3_mem_NodeHeader* node)
{
	return &(node->right);
}

a3_mem_NodeHeader** a3_mem_getParentReference(a3_mem_NodeHeader* node)
{
	return node->parent->left == node ? &(node->parent->left) : &(node->parent->right);
}

a3_mem_NodeHeader* a3_mem_getSmallestNode(a3_mem_NodeHeader* node)
{
	a3_mem_NodeHeader* currentNode = node;
	while (currentNode != 0 && currentNode->left != 0)
	{
		currentNode = currentNode->left;
	}
	return currentNode;
}

char a3_mem_getHeightSafe(a3_mem_NodeHeader const * const node)
{
	return node == 0 ? 0 : node->height;
}

char a3_mem_getBalanceFactor(a3_mem_NodeHeader const * const node)
{
	return node == 0 ? 0 : a3_mem_getHeightSafe(node->right) - a3_mem_getHeightSafe(node->left);
}

char a3_mem_calculateHeight(a3_mem_NodeHeader const * const node)
{
	return maximum(a3_mem_getHeightSafe(node->left), a3_mem_getHeightSafe(node->right)) + 1;
}

void a3_mem_calculateHeightOfAncestry(a3_mem_NodeHeader* node)
{
	while (node != 0)
	{
		node->height = a3_mem_calculateHeight(node);
		node = node->parent;
	}
}

// AVL Tree: Management ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void a3_mem_addNodeToFreeTree(a3_mem_ChunkHeader* chunkHeader, a3_mem_NodeHeader* node)
{

	// Clear all AVL tree vars, just in case they are set for some reason
	node->parent = 0;
	node->left = 0;
	node->right = 0;
	node->height = 0;
	
	a3_mem_attachNodeToTree(chunkHeader, node);

	// Update the heights of parent nodes and balance as necessary
	a3_mem_NodeHeader* currentNode = node;
	while (currentNode != 0)
	{
		currentNode->height = a3_mem_calculateHeight(currentNode);

		// Balance the subtree if required
		if (compare_absolute(a3_mem_getBalanceFactor(currentNode)) > 1)
		{
			a3_mem_balanceSubtree(chunkHeader, currentNode);
		}

		currentNode = currentNode->parent;
	}

}

a3_mem_NodeHeader* a3_mem_findParentForNodeInsertion(a3_mem_NodeHeader* root, a3_mem_NodeHeader* node)
{
	a3_mem_NodeHeader* prevNode = 0;
	a3_mem_NodeHeader* currentNode = root;

	// Go until prevNode is a leaf node or has no child on the desired side
	while (currentNode != 0)
	{
		prevNode = currentNode;

		// nodeSize < currentNodeSize goes on the left
		if (node->size < currentNode->size)
		{
			currentNode = currentNode->left;
		}
		// nodeSize >= currentNodeSize goes on the right
		else
		{
			currentNode = currentNode->right;
		}
	}

	return prevNode;
}

void a3_mem_attachNodeToTree(a3_mem_ChunkHeader* chunkHeader, a3_mem_NodeHeader* node)
{
	// Assign the parent node
	// Performance: binary search for insertion, what ever that is...
	node->parent = a3_mem_findParentForNodeInsertion(chunkHeader->freeNodeRoot, node);

	if (node->parent != 0)
	{
		// Link node to the correct side of the paren
		if (node->size < node->parent->size)
		{
			node->parent->left = node;
		}
		else
		{
			node->parent->right = node;
		}
	}
	else if (chunkHeader->freeNodeRoot == 0)
	{
		chunkHeader->freeNodeRoot = node;
	}
}

void a3_mem_removeNodeFromFreeTree(a3_mem_ChunkHeader* chunkHeader, a3_mem_NodeHeader* node)
{

	if (chunkHeader->freeNodeRoot == node)
	{
		chunkHeader->freeNodeRoot = 0;
	}

	// check number of children
	int numberOfNodes = 0;
	if (node->left != 0) numberOfNodes++;
	if (node->right != 0) numberOfNodes++;

	switch (numberOfNodes)
	{
	case 0:
		// if no children then just remove it
		a3_mem_connectFreeNodeToParent(chunkHeader, node, 0);
		a3_mem_calculateHeightOfAncestry(node->parent);
		break;
	case 1:
		// if one child then move child into the spot of the removed node
	{
		// find which child node exists
		a3_mem_NodeHeader *childNode = node->left != 0 ? node->left : node->right;

		// set childNode's parent pointing to removed node's parent
		childNode->parent = node->parent;

		a3_mem_connectFreeNodeToParent(chunkHeader, node, childNode);
		a3_mem_calculateHeightOfAncestry(node->parent);

		break;
	}
	case 2:
		// if two children then get the left most child of the removed child and move it into the removed node slot
	{
		// get left most node of the removed node's right node
		a3_mem_NodeHeader *leftMostNode = a3_mem_getSmallestNode(node->right);
		a3_mem_NodeHeader *oneAbove = leftMostNode->parent;

		// First make sure the right node of leftMostNode is connected to the leftMostNode's parent node
		// check to see if right node is the node we are using (means that the removed node and leftMostNode are parent and child nodes)
		if (node->right != leftMostNode)
		{
			if (leftMostNode->right != 0)
				leftMostNode->right->parent = leftMostNode->parent;
			leftMostNode->parent->left = leftMostNode->right;

			// checks if leftMostNode is right below the node we are removing
			// if it is then the leftMostNode's right does not need to be reconnected
			leftMostNode->right = node->right;
			leftMostNode->right->parent = leftMostNode;
		}

		// Now reconnect leftMostNode to the removed nodes connections

		// switch new node's parent for removed node's parent
		leftMostNode->parent = node->parent;

		// set new left node connection
		leftMostNode->left = node->left;
		leftMostNode->left->parent = leftMostNode;

		// set up parent connections
		a3_mem_connectFreeNodeToParent(chunkHeader, node, leftMostNode);

		if (oneAbove != node)
		{
			leftMostNode->height = a3_mem_calculateHeight(leftMostNode);
		}
		else
		{
			a3_mem_calculateHeightOfAncestry(oneAbove);
		}

		break;
	}
	default:
		break;
	}

	// Clear AVL vars in case they are still set
	node->parent = 0;
	node->left = 0;
	node->right = 0;
	node->height = 0;

}

void a3_mem_mergeFreeNodes(a3_mem_ChunkHeader* chunkHeader, a3_mem_NodeHeader* first, a3_mem_NodeHeader* second)
{
	// Remove both nodes from the AVL tree
	a3_mem_removeNodeFromFreeTree(chunkHeader, first);
	if (second->isAllocated == 0) a3_mem_removeNodeFromFreeTree(chunkHeader, second);

	// Update the size of the first node
	first->size += sizeof(a3_mem_NodeHeader) + second->size;

	// Now that the size of the first is updated
	// Get the next node in linear memory and set its previous to the first node
	a3_mem_NodeHeader* newNext = a3_mem_getNextNode(chunkHeader, first);
	if (newNext != 0) newNext->prev = first;

	// Add the node earlier in linear memory back to the AVL tree
	a3_mem_addNodeToFreeTree(chunkHeader, first);
}

void a3_mem_connectFreeNodeToParent(a3_mem_ChunkHeader* chunkHeader, a3_mem_NodeHeader* node, a3_mem_NodeHeader* childNode)
{
	// check to see if root node
	if (node->parent != 0)
	{
		// determine if the node being removed is the left or right child
		// and redirect the parent's pointer to our child
		*(a3_mem_getParentReference(node)) = childNode;
	}
	else
	{
		// if root node then root node point to the new node header
		chunkHeader->freeNodeRoot = childNode;
		if (childNode != 0)
		{
			childNode->parent = 0;
		}
	}
}

// AVL Tree: Balacing/Rotations ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void a3_mem_balanceSubtree(a3_mem_ChunkHeader*const chunkHeader, a3_mem_NodeHeader *const node)
{
	char bf = node == 0 ? 0 : a3_mem_getBalanceFactor(node);
	a3_mem_NodeHeader** rootPtr = &(chunkHeader->freeNodeRoot);

	if (bf < 0)
	{
		if (a3_mem_getBalanceFactor(node->left) == -1)
		{
			a3_mem_rotateRight(node, rootPtr);
		}
		else
		{
			a3_mem_rotateLeftRight(node, rootPtr);
		}
	}
	else if (bf > 0)
	{
		if (a3_mem_getBalanceFactor(node->left) == -1)
		{
			a3_mem_rotateRightLeft(node, rootPtr);
		}
		else
		{
			a3_mem_rotateLeft(node, rootPtr);
		}
	}

	/* NOTE: Could make the previous block of code smaller, but it results in code that is harder to understand and maintain. See note GOTO_ROTATE.
	// could theoreticall mix the function pointers via a clamped lerp
	//bf = a3clamp(-1, 1, bf);

	RotationFunction rotate = 0;

	if (bf < 0)
	{
		rotate = a3_mem_pickRotation(node->left, &a3_mem_rotateRight, &a3_mem_rotateLeftRight);
	}
	else if (bf > 0)
	{
		rotate = a3_mem_pickRotation(node->right, &a3_mem_rotateRightLeft, &a3_mem_rotateLeft);
	}

	if (rotate != 0)
		rotate(node, &(chunkHeader->freeNodeRoot));
	//*/

}

void a3_mem_rotateLeft(a3_mem_NodeHeader *const node, a3_mem_NodeHeader** root)
{
	// NOTE: Could condense this to a common function, but see note GOTO_ROTATE. 
	// a3_mem_rotate(node, root, &a3_mem_getReferenceToChildLeft, &a3_mem_getReferenceToChildRight);
	
	a3_mem_NodeHeader *child = node->right;
	a3_mem_NodeHeader *parent = node->parent;

	node->right = child->left;
	if (node->right != 0)
		node->right->parent = node;

	child->left = node;
	child->left->parent = child;

	child->parent = parent;
	if (parent != 0)
	{
		if (parent->left == node) parent->left = child;
		else parent->right = child;
	}
	else
	{
		*root = child;
	}

	node->height = a3_mem_calculateHeight(node);
	child->height = a3_mem_calculateHeight(child);
}

void a3_mem_rotateRight(a3_mem_NodeHeader *const node, a3_mem_NodeHeader** root)
{
	// NOTE: Could condense this to a common function, but see note GOTO_ROTATE. 
	// a3_mem_rotate(node, root, &a3_mem_getReferenceToChildRight, &a3_mem_getReferenceToChildLeft);

	a3_mem_NodeHeader *child = node->left;
	a3_mem_NodeHeader *parent = node->parent;

	node->left = child->right;
	if (node->left != 0)
		node->left->parent = node;

	child->right = node;
	child->right->parent = child;

	child->parent = parent;
	if (parent != 0)
	{
		if (parent->left == node) parent->left = child;
		else parent->right = child;
	}
	else
	{
		*root = child;
	}

	node->height = a3_mem_calculateHeight(node);
	child->height = a3_mem_calculateHeight(child);
}

// NOTE: GOTO_ROTATE: This is an example of code which could be condensed, but doing so makes it more difficult to understand
void a3_mem_rotate(a3_mem_NodeHeader *const node, a3_mem_NodeHeader** root, GetChildBranch getBranchAligned, GetChildBranch getBranchOpposite)
{
	// For a RIGHT rotation,
	//    getBranchAligned will return a pointer reference for the RIGHT child,
	//    getBranchOpposite will return a pointer reference for the LEFT child.
	// For a LEFT rotation,
	//    getBranchAligned will return a pointer reference for the LEFT child,
	//    getBranchOpposite will return a pointer reference for the RIGHT child.

	// &( R: node->left, L: node->right )
	a3_mem_NodeHeader** nodePtrOpposite = getBranchOpposite(node);

	// child=(R: node->left, L: node->right)
	a3_mem_NodeHeader *child = *nodePtrOpposite;
	// &( R: child->right, L: child->left )
	a3_mem_NodeHeader** childPtrAligned = getBranchAligned(child);

	a3_mem_NodeHeader *parent = node->parent;
	// &( R: parent->right, L: parent->left )
	a3_mem_NodeHeader** parentPtrAligned = getBranchAligned(parent);
	// &( R: parent->left, L: parent->right )
	a3_mem_NodeHeader** parentPtrOpposite = getBranchOpposite(parent);

	// R: node->left = child->right
	// L: node->right = child->left
	*nodePtrOpposite = *childPtrAligned;
	// R: node->left
	// L: node->right
	if (*nodePtrOpposite != 0)
		(*nodePtrOpposite)->parent = node;

	// R: child->right = node
	// L: child->left = node
	*childPtrAligned = node;
	// R: child->right->parent = child;
	// L: child->left->parent = child;
	(*childPtrAligned)->parent = child;

	child->parent = parent;
	if (parent != 0)
	{
		// R: if (parent->right == node) parent->right = child; else parent->left = child;
		// L: if (parent->left == node) parent->left = child; else parent->right = child;
		*(*parentPtrAligned == node ? parentPtrAligned : parentPtrOpposite) = child;
	}
	else
	{
		*root = child;
	}

	node->height = a3_mem_calculateHeight(node);
	child->height = a3_mem_calculateHeight(child);
}

void a3_mem_rotateLeftRight(a3_mem_NodeHeader *const node, a3_mem_NodeHeader** root)
{
	a3_mem_rotateLeft(node->left, root);
	a3_mem_rotateRight(node, root);
}

void a3_mem_rotateRightLeft(a3_mem_NodeHeader *const node, a3_mem_NodeHeader** root)
{
	a3_mem_rotateRight(node->right, root);
	a3_mem_rotateLeft(node, root);
}

RotationFunction a3_mem_pickRotation(a3_mem_NodeHeader* secondaryNode, RotationFunction negative, RotationFunction positive)
{
	return a3_mem_getBalanceFactor(secondaryNode) == -1 ? negative : positive;
}

// AVL Tree: Search ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

a3_mem_NodeHeader* a3_mem_findSmallestNodeWithMinimumSize(a3_mem_ChunkHeader* chunkHeader, uSize totalSize)
{
	a3_mem_NodeHeader* currentNode = chunkHeader->freeNodeRoot;
	a3_mem_NodeHeader* bestNode = 0;

	while (currentNode != 0)
	{
		// If the current node is not large enough to hold the requested size
		if (totalSize > currentNode->size)
		{
			// Must be larger, get the one to the right in the AVL tree
			currentNode = currentNode->right;
		}
		else
		{
			// totalSize <= node size
			// If best node is current nil, this is the best node
			// otherwise the best node is the one with the smallest size
			// If the sizes are equal, choose the one that is the left-most
			if (bestNode == 0)
			{
				bestNode = currentNode;
			}
			else if (currentNode->size < bestNode->size)
			{
				bestNode = currentNode;
			}
			else if (currentNode < bestNode)
			{
				bestNode = currentNode;
			}

			// Next best option is anything that is smaller
			currentNode = currentNode->left;
		}
	}

	return bestNode;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
