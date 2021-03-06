/**
 * @file    CircularBuffer.h
 * @author  Jonathan Simmonds
 * @brief     Very fast implementation of a Circular Buffer.
 */
#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

/*-------------------- INCLUDES --------------------*/
//#include "stdafx.h"
//#include "SharedIncludes.h"
#include <stdint.h>


/*-------------------- CLASS DEFINITIONS --------------------*/
/**  
 * @brief     Very fast implementation of a Circular Buffer.
 *
 *          NB: To maximise speed a minimum amount of bound-checking is done, please
 *          ensure you understand how to use the functions beforehand.
 *          A circular buffer is used to implement a queue when the queue has a fixed
 *          maximum size. Once space in the buffer is always left empty for technical
 *          reasons, so ensure that this is accounted for when the bufferSize is 
 *          decided upon (i.e. maximumCapacity = bufferSize - 1). All operations on
 *          the Circular Buffer can be performed in constant time.
 *          For further information on circular buffers, see:
 *          http://en.wikipedia.org/wiki/Circular_buffer
*/
template <class T>
class CircularBuffer
{
public:
    /// @brief  Constructor, setting the CircularBuffer's variables.
    /// @param  bs  The buffer size. Once initialised this is not changeable.
    CircularBuffer (uint8_t bs = 32) : bufferSize(bs), bufferSizeMinusTwo(bs - 2), head(0), tail(0)
    {
        buffer = (T*) malloc(bs * sizeof(T));
    }

    
    /// @brief  Deconstructor.
    ~CircularBuffer (void)
    {
        free(buffer);
    }

    
    /// @brief  Returns whether or not the buffer is full.
    ///         When full the buffer will insert new elements over the oldest.
    /// @return The state of the buffer.
    virtual inline bool isFull (void) const
    {
        return (sMod(head + 1) == tail);
    }
    

    /// @brief  Returns whether or not the buffer is empty.
    /// @return The state of the buffer.
    virtual inline bool isEmpty (void) const
    {
        return (head == tail);
    }

    virtual inline uint8_t getSize (void) const
    {
        return (head < tail) ? (head + bufferSize) - tail : head - tail;
    }
    
    /// @brief  Treats the buffer like an array (elements 0, 1, ..., bufferSize-1) and returns the item from
    ///         the buffer located at the given index.
    ///         Please note, the index MUST BE VALID (i.e. index >= 0,   index < bufferSize-2 (as the buffer
    ///         always has 1 empty space for technical reasons)).
    /// @param  index   The index of the target element in the virtual 'array'.
    /// @return A pointer to the target element.
    virtual T* get (const uint8_t index) const    
    {
        return &(buffer[sMod(tail + index)]);
    }

    
    /// @brief  Adds an item to the buffer, overwriting the oldest item if the buffer is full.
    /// @param  item  The item to add to the buffer.
    virtual void add (const T item)
    {
        //char bob[128];
        //sprintf(bob, "buffer = %p, head = %d, buffer[head] = %p, buffersize = %d, sizeof(T) = %d\n", buffer, head, &(buffer[head]), bufferSize, sizeof(T));
        //OutputDebugString(bob);
        buffer[head] = item;
        head = sMod(head + 1);
        if (head == tail)
            tail = sMod(tail + 1);
    }

    
    /// @brief  Removes the oldest item (if one exists) from the buffer.
    virtual void remove (void)
    {
        if (!isEmpty())
            tail = sMod(tail + 1);
    }



protected:
    /// @brief  sMod performs the calculation (x % bufferSize) where 0 <= x < 2*bufferSize.
    /**         The function performs the calculation much faster than the modulo function, however
                it is important that the two restrictions on x are observed. */
    /// @param  x   x in the calculation (x % bufferSize).
    /// @return The solution to the calculation (x % bufferSize).
    inline uint8_t sMod (const uint8_t x) const
    {
        return x < bufferSize ? x : x - bufferSize;
    }

    const uint8_t bufferSize;           ///< The buffer size.
    const uint8_t bufferSizeMinusTwo;   ///< bufferSize - 2. Included to further optimise the code.
    T* buffer;      ///< The actual buffer.
    uint8_t head;   ///< The buffer head: always points to a blank (or no-longer accessible) element.
    uint8_t tail;   ///< The buffer tail: always points to the oldest element.
};

#endif // #ifndef CIRCULARBUFFER_H

