/*
 * Copyright (C) 2009 Codership Oy <info@codership.com>
 *
 * $Id$
 */

/*!
 * @file Input map for EVS messaging. Provides simple interface for 
 *       handling messages with different safety guarantees.
 *
 * @note When operating with iterators, note that evs::Message 
 *       accessed through iterator may have different sequence 
 *       number as it position dictates. Use sequence number 
 *       found from key part.
 *
 * @todo Fix issue in above note if feasible.
 */

#ifndef EVS_INPUT_MAP2_HPP
#define EVS_INPUT_MAP2_HPP

#include "evs_message2.hpp"
#include "profile.hpp"
#include "gcomm/map.hpp"

#include <vector>

namespace gcomm
{
    /* Forward declarations */
    class ReadBuf;
    class InputMapMsgKey;
    std::ostream& operator<<(std::ostream&, const InputMapMsgKey&);
    namespace evs
    {
        class InputMapMsg;
        std::ostream& operator<<(std::ostream&, const InputMapMsg&);
        class InputMapMsgIndex;
        class InputMapNode;
        std::ostream& operator<<(std::ostream&, const InputMapNode&);
        typedef std::vector<InputMapNode> InputMapNodeIndex;
        std::ostream& operator<<(std::ostream&, const InputMapNodeIndex&);
        class InputMap;
    }
}

/* Internal msg representation */
class gcomm::InputMapMsgKey
{
public:
    InputMapMsgKey(const size_t index_, const evs::Seqno seq_) : 
        index(index_), 
        seq(seq_) 
    { }
    
    size_t     get_index() const { return index; }
    evs::Seqno get_seq  () const { return seq;   }

    bool operator<(const InputMapMsgKey& cmp) const
    {
        return (seq < cmp.seq || (seq == cmp.seq && index < cmp.index));
    }

private:
    size_t      const index;
    evs::Seqno  const seq;
};


        
/* Internal message representation */
class gcomm::evs::InputMapMsg
{
public:
    InputMapMsg(const UserMessage&       msg_, 
                ReadBuf*           const rb_   ) :
        msg  (msg_ ),
        rb   (rb_  )
    { }
    
    InputMapMsg(const InputMapMsg& m) :
        msg  (m.msg ),
        rb   (m.rb  )
    { }
            
    const UserMessage& get_msg () const { return msg;  }
    const ReadBuf*     get_rb  () const { return rb;   }
    ReadBuf*           get_rb  ()       { return rb;   }

private:
    void operator=(const InputMapMsg&);
    
    UserMessage const msg;
    ReadBuf*    const rb;
};



class gcomm::evs::InputMapMsgIndex : 
    public Map<InputMapMsgKey, InputMapMsg> { };


/* Internal node representation */
class gcomm::evs::InputMapNode
{
public:
    InputMapNode() : idx(), range(0, Seqno::max()), safe_seq() { }

    void   set_range     (const Range  r)       { range     = r; }
    void   set_safe_seq  (const Seqno  s)       { safe_seq  = s; }
    void   set_index     (const size_t i)       { idx       = i; }
    
    Range  get_range     ()               const { return range;     }
    Seqno  get_safe_seq  ()               const { return safe_seq;  }
    size_t get_index     ()               const { return idx;       }

private:
    size_t idx;
    Range range;
    Seqno safe_seq;
};

        



/*!
 * Input map for messages.
 *
 */
class gcomm::evs::InputMap
{
public:                

    /* Iterators exposed to user */
    typedef InputMapMsgIndex::iterator iterator;
    typedef InputMapMsgIndex::const_iterator const_iterator;
            
    /*!
     * Default constructor.
     */
    InputMap();
    
    /*!
     * Default destructor.
     */
    ~InputMap();
            
    /*!
     * Get current value of aru_seq.
     *
     * @return Current value of aru_seq
     */
    Seqno get_aru_seq () const { return aru_seq;  }

    /*!
     * Get current value of safe_seq. 
     *
     * @return Current value of safe_seq
     */
    Seqno get_safe_seq() const { return safe_seq; }
    
    /*!
     * Set sequence number safe for node.
     *
     * @param uuid Node uuid
     * @param seq Sequence number to be set safe
     *
     * @throws FatalException if node was not found or sequence number
     *         was not in the allowed range
     */
    void  set_safe_seq(const size_t uuid, const Seqno seq)
        throw (gu::Exception);
    
    /*!
     * Get current value of safe_seq for node.
     *
     * @param uuid Node uuid
     *
     * @return Safe sequence number for node
     *
     * @throws FatalException if node was not found
     */
    Seqno get_safe_seq(const size_t uuid) const 
        throw (gu::Exception)
    {
        return node_index->at(uuid).get_safe_seq();
    }
    
    /*!
     * Get current range parameter for node
     *
     * @param uuid Node uuid
     * 
     * @return Range parameter for node
     *
     * @throws FatalException if node was not found
     */
    Range get_range   (const size_t uuid) const 
        throw (gu::Exception)
    {
        return node_index->at(uuid).get_range();
    }
    
    Seqno get_min_hs() const
        throw (gu::Exception);
    
    Seqno get_max_hs() const
        throw (gu::Exception);
    
    /*!
     * Get iterator to the beginning of the input map
     *
     * @return Iterator pointing to the first element
     */
    iterator begin() const { return msg_index->begin(); }
    
    /*!
     * Get iterator next to the last element of the input map
     *
     * @return Iterator pointing past the last element
     */
    iterator end  () const { return msg_index->end(); }
     
    /*!
     * Check if message pointed by iterator fulfills SP_SAFE condition.
     *
     * @return True or false
     */
    bool is_safe  (iterator i) const 
        throw (gu::Exception)
    {
        const Seqno seq(InputMapMsgIndex::get_key(i).get_seq());
        return (safe_seq != Seqno::max() && seq <= safe_seq);
    }
    
    /*!
     * Check if message pointed by iterator fulfills SP_AGREED condition.
     *
     * @return True or false
     */
    bool is_agreed(iterator i) const 
        throw (gu::Exception)
    {
        const Seqno seq(InputMapMsgIndex::get_key(i).get_seq());
        return (aru_seq != Seqno::max() && seq <= aru_seq);
    }
    
    /*!
     * Check if message pointed by iterator fulfills SP_FIFO condition.
     *
     * @return True or false
     */
    bool is_fifo  (iterator i) const
        throw (gu::Exception)
    {
        const Seqno seq(InputMapMsgIndex::get_key(i).get_seq());
        const InputMapNode& node(node_index->at(
                                     InputMapMsgIndex::get_key(i).get_index()));
        return (node.get_range().get_lu() > seq);
    }

    bool has_deliverables() const
    {
        if (msg_index->empty() == false)
        {
            if (n_msgs[SP_FIFO] > 0 && is_fifo(msg_index->begin()))
                return true;
            else if (n_msgs[SP_AGREED] > 0 && is_agreed(msg_index->begin()))
                return true;
            else if (n_msgs[SP_SAFE] > 0 && is_safe(msg_index->begin()))
                return true;
            else if (n_msgs[SP_DROP] > max_droppable)
                return true;
            return false;
        }
        else
        {
            return false;
        }
    }
    
    /*!
     * Insert new message into input map.
     *
     * @param uuid   Node uuid of the message source
     * @param msg    EVS message 
     * @param rb     ReadBuf pointer associated to message
     * @param offset Offset to the beginning of the payload
     *
     * @return Range parameter of the node
     *
     * @throws FatalException if node not found or message sequence 
     *         number is out of allowed range
     */
    Range insert(const size_t uuid, const UserMessage& msg, 
                 const ReadBuf* rb = 0, size_t offset = 0)
        throw (gu::Exception);
    
    /*!
     * Erase message pointed by iterator. Note that message may still
     * be recovered through recover() method as long as it does not 
     * fulfill SP_SAFE constraint.
     *
     * @param i Iterator
     *
     * @throws FatalException if iterator is not valid
     */
    void erase(iterator i)
        throw (gu::Exception);
    
    /*!
     * Find message.
     *
     * @param uuid Message source node uuid
     * @param seq  Message sequence numeber
     *
     * @return Iterator pointing to message or at end() if message was not found
     *
     * @throws FatalException if node was not found
     */
    iterator find(const size_t uuid, const Seqno seq) const
        throw (gu::Exception);
    
    /*!
     * Recover message.
     *
     * @param uuid Message source node uuid
     * @param seq  Message sequence number
     *
     * @return Iterator pointing to the message
     *
     * @throws FatalException if node or message was not found
     */
    iterator recover(const size_t uuid, const Seqno seq) const
        throw (gu::Exception);
    
    /*!
     *
     */
    void reset(const size_t, const size_t = 256)
        throw (gu::Exception);
    
    /*!
     * Clear input map state.
     */
    void clear();
    
    
private:
    friend std::ostream& operator<<(std::ostream&, const InputMap&);    
    /* Non-copyable */
    InputMap(const InputMap&);
    void operator=(const InputMap&);
            
    /*! 
     * Update aru_seq value to represent current state.
     */
    void update_aru()
        throw (gu::Exception);
            
    /*!
     * Clean up recovery index. All messages up to safe_seq are removed.
     */
    void cleanup_recovery_index()
        throw (gu::Exception);
    
    Seqno              safe_seq;       /*!< Safe seqno              */
    Seqno              aru_seq;        /*!< All received upto seqno */
    InputMapNodeIndex* node_index;     /*!< Index of nodes          */
    InputMapMsgIndex*  msg_index;      /*!< Index of messages       */
    InputMapMsgIndex*  recovery_index; /*!< Recovery index          */
    
    std::vector<size_t> n_msgs;
    size_t max_droppable;

    prof::Profile prof;
};

#endif // EVS_INPUT_MAP2_HPP
