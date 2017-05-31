/*******************************************************************************
 * Copyright (c) 2016 CEA LIST.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Created on: 3 févr. 2015
 *
 * Contributors:
 *  Arnault Lapitre (CEA LIST) arnault.lapitre@cea.fr
 *   - Initial API and implementation
 ******************************************************************************/

#ifndef MULTILIFOBUFFER_H_
#define MULTILIFOBUFFER_H_

#include <fml/buffer/BaseBufferQueue.h>

namespace sep
{


class MultiLifoBuffer : public BaseBufferQueue
{

	AVM_DECLARE_CLONABLE_CLASS( MultiLifoBuffer )


public:
	/**
	 * CONSTRUCTOR
	 * Default
	 */
	MultiLifoBuffer(InstanceOfBuffer * aBuffer)
	: BaseBufferQueue(CLASS_KIND_T( MultiLifoBuffer ), aBuffer)
	{
		//!! NOTHING
	}

	/**
	 * CONSTRUCTOR
	 * Copy
	 */
	MultiLifoBuffer(const MultiLifoBuffer & aMultiLifo )
	: BaseBufferQueue( aMultiLifo )
	{
		//!! NOTHING
	}


	////////////////////////////////////////////////////////////////////////////
	// BUFFER MANAGEMENT API
	////////////////////////////////////////////////////////////////////////////
	/**
	 * push
	 * top
	 */
	inline virtual bool push(const Message & aMsg)
	{
		if( size() < capacity() )
		{
			mMessages.push_back(aMsg);

			return( true );
		}

		return( false );
	}


	inline virtual bool top(const Message & aMsg)
	{
		if( nonempty() )
		{
			mMessages.back() = aMsg;

			return( true );
		}

		return( false );
	}

	inline virtual const Message & top() const
	{
		if( nonempty() )
		{
			return( mMessages.back() );
		}

		return( Message::_NULL_ );
	}

	inline virtual const Message & top(avm_size_t mid,
			const RuntimeID & aReceiverRID = RuntimeID::REF_NULL) const
	{
		if( nonempty() && mMessages.back().isCompatible(mid, aReceiverRID) )
		{
			return( mMessages.back() );
		}

		return( Message::_NULL_ );
	}


	/**
	 * contains
	 * uncontains
	 */
	inline virtual bool contains(ListOfInstanceOfPort & aSignalTrace,
			const RuntimeID & aReceiverRID = RuntimeID::REF_NULL) const
	{
		ListOfInstanceOfPort::const_iterator itSignal = aSignalTrace.begin();
		ListOfInstanceOfPort::const_iterator endSignal = aSignalTrace.end();
		ListOfMessage::const_reverse_iterator it = mMessages.rbegin();
		ListOfMessage::const_reverse_iterator itEnd = mMessages.rend();
		for( ; (it != itEnd) && (itSignal != endSignal) ; ++it )
		{
			if( (*it).isCompatible(*itSignal, aReceiverRID) )
			{
				++itSignal;
			}
		}

		return( itSignal == endSignal );
	}


	inline virtual bool uncontains(ListOfInstanceOfPort & aSignalTrace,
			const RuntimeID & aReceiverRID = RuntimeID::REF_NULL) const
	{
		ListOfInstanceOfPort::iterator itSignal = aSignalTrace.begin();
		ListOfInstanceOfPort::iterator endSignal = aSignalTrace.end();
		ListOfMessage::const_reverse_iterator it = mMessages.rbegin();
		ListOfMessage::const_reverse_iterator itEnd = mMessages.rend();
		for( ; (it != itEnd) && (itSignal != endSignal) ; ++it )
		{
			if( (*it).isCompatible(*itSignal, aReceiverRID) )
			{
				itSignal = aSignalTrace.erase( itSignal );
			}
		}

		return( aSignalTrace.nonempty() );
	}


	/**
	 * pop
	 */
	inline virtual Message pop()
	{
		if( nonempty() )
		{
			Message aMsg = mMessages.pop_last();

			return( aMsg );
		}

		return( Message::_NULL_ );
	}


	inline virtual Message pop(avm_size_t mid,
			const RuntimeID & aReceiverRID = RuntimeID::REF_NULL)
	{
		if( nonempty() )
		{
			Message aMsg = mMessages.back();
			if( aMsg.isCompatible(mid, aReceiverRID) )
			{
				mMessages.pop_back();

				return( aMsg );
			}
		}

		return( Message::_NULL_ );
	}



	inline virtual void popBefore(const RuntimeID & aReceiverRID)
	{
		ListOfMessage::reverse_iterator it = mMessages.rbegin();
		for( ; it != mMessages.rend() ; )
		{
			++it;

			if( (*it).isCompatible(aReceiverRID) )
			{
				mMessages.erase( it.base() );
			}
		}
	}

	inline virtual void popBefore(const ListOfInstanceOfPort & ieComs,
			const RuntimeID & aReceiverRID = RuntimeID::REF_NULL)
	{
		ListOfMessage::reverse_iterator it = mMessages.rbegin();
		for( ; it != mMessages.rend() ; )
		{
			++it;

			if( (*it).isCompatible(aReceiverRID) )
			{
				if( (*it).isCompatible(ieComs) )
				{
					break;
				}
				else
				{
					mMessages.erase( it.base() );
				}
			}
		}
	}

	inline virtual void popBefore(const ListOfSizeT & ieComs,
			const RuntimeID & aReceiverRID = RuntimeID::REF_NULL)
	{
		ListOfMessage::reverse_iterator it = mMessages.rbegin();
		for( ; it != mMessages.rend() ; )
		{
			++it;

			if( (*it).isCompatible(aReceiverRID) )
			{
				if( (*it).isCompatible(ieComs) )
				{
					break;
				}
				else
				{
					mMessages.erase( it.base() );
				}
			}
		}
	}


	/**
	 * copyTo
	 * restore
	 */
	inline virtual void copyTo(BaseBufferForm & aBuffer) const
	{
		ListOfMessage::const_reverse_iterator it = mMessages.rbegin();
		ListOfMessage::const_reverse_iterator itEnd = mMessages.rend();
		for( ; it != itEnd ; ++it )
		{
			aBuffer.push( *it );
		}
	}

	inline virtual void restore(ListOfMessage & listOfMessage)
	{
		while( listOfMessage.nonempty() )
		{
			mMessages.push_back( listOfMessage.back() );

			listOfMessage.pop_back();
		}
	}

};


} /* namespace sep */

#endif /* MULTILIFOBUFFER_H_ */
