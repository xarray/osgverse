//
// Created by Gianmarco Cherchi on 07/03/24.
//

#ifndef MESH_ARRANGEMENT_CUSTOM_STACK_H
#define MESH_ARRANGEMENT_CUSTOM_STACK_H

#include <vector>
#include <cassert>
#include "aux_structure.h"

class CustomStack
{
public:
    CustomStack(int preallocate_size)
    {
        stack.resize(preallocate_size);
        cursor = -1;
    }

    auxvector<uint>& pop()
    {
        cursor -= 1;
        return stack.at(cursor +1);
    }

    void push(auxvector<uint> new_vec)
    {
        if(cursor == stack.size() -1)
            stack.push_back(new_vec);
        else
            stack[cursor +1] = new_vec;

        cursor++;
    }

    bool empty()
    {
        return cursor == -1;
    }

    const std::vector<auxvector<uint>> &getStack(int &size)
    {
        size = stack.size();
        return stack;
    }

    auxvector<uint> &getSingleVector(int index)
    {
        assert(index <= cursor && "Index out of range");
        return stack[index];
    }

    void clearSingleVector(int index)
    {
        assert(index <= cursor && "Index out of range");
        stack.at(index).clear();
    }

    int findTriplet(uint v0, uint v1, uint v2)
    {
        for(int i = cursor; i >= 0; --i)
        {
            assert(stack[i].size() >= 3 && "Empty element in the queue");

            if ((stack[i][0] == v0 && stack[i][1] == v1 && stack[i][2] == v2) ||
                (stack[i][0] == v0 && stack[i][1] == v2 && stack[i][2] == v1) ||
                (stack[i][0] == v1 && stack[i][1] == v0 && stack[i][2] == v2) ||
                (stack[i][0] == v1 && stack[i][1] == v2 && stack[i][2] == v0) ||
                (stack[i][0] == v2 && stack[i][1] == v0 && stack[i][2] == v1) ||
                (stack[i][0] == v2 && stack[i][1] == v1 && stack[i][2] == v0) ){
                return i;
            }
        }

        assert(false && "Triplet not found!");
        return -1; // Triplet not found
    }

    const auxvector<uint>& getTriangleFromStack(uint v0, uint v1, uint v2)
    {
        for(int i = cursor; i >= 0; --i)
        {
            assert(stack[i].size() >= 3 && "Empty element in the queue");

            if ((stack[i][0] == v0 && stack[i][1] == v1 && stack[i][2] == v2) ||
                (stack[i][0] == v0 && stack[i][1] == v2 && stack[i][2] == v1) ||
                (stack[i][0] == v2 && stack[i][1] == v1 && stack[i][2] == v0) ||
                (stack[i][0] == v1 && stack[i][1] == v0 && stack[i][2] == v2) ||
                (stack[i][0] == v1 && stack[i][1] == v2 && stack[i][2] == v0) ||
                (stack[i][0] == v2 && stack[i][1] == v0 && stack[i][2] == v1)){
                return stack[i];
            }
        }

        assert(false && "Triplet not found!");
        return auxvector<uint>(); // Triplet not found
    }



private:
        std::vector<auxvector<uint>> stack;
        int cursor;
};


#endif //MESH_ARRANGEMENT_CUSTOM_STACK_H
