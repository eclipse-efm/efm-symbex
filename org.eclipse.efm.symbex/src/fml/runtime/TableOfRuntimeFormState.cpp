/*******************************************************************************
 * Copyright (c) 2016 CEA LIST.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Created on: 13 juil. 2008
 *
 * Contributors:
 *  Arnault Lapitre (CEA LIST) arnault.lapitre@cea.fr
 *   - Initial API and implementation
 ******************************************************************************/

#include "TableOfRuntimeFormState.h"

#include <fml/runtime/ExecutionData.h>
#include <fml/runtime/RuntimeDef.h>


namespace sep
{


/**
 * RESIZE
 * mEvalState
 * mTableOfAssignedFlag
 */
void TableOfRuntimeFormState::resize(avm_size_t aSize)
{
	if( mSize > 0 )
	{
		PROCESS_EVAL_STATE * oldEvalState = mEvalState;

		avm_size_t offset = mSize;
		mSize = aSize;

		if( aSize > offset )
		{
			aSize = offset;
		}

		mEvalState = new PROCESS_EVAL_STATE[ mSize ];

		if( mTableOfAssignedFlag != NULL )
		{
			TableOfAssignedFlag oldTableOfAssigned = mTableOfAssignedFlag;
			mTableOfAssignedFlag = new Bitset *[ mSize ];

			for( offset = 0 ; offset < aSize ; ++offset )
			{
				mEvalState[ offset ] = oldEvalState[ offset ];

				mTableOfAssignedFlag[ offset ] = oldTableOfAssigned[ offset ];
			}

			for( ; offset < mSize ; ++offset )
			{
				mEvalState[ offset ] = PROCESS_UNDEFINED_STATE;

				mTableOfAssignedFlag[ offset ] = NULL;
			}

			delete [] oldTableOfAssigned;
		}
		else
		{
			for( offset = 0 ; offset < aSize ; ++offset )
			{
				mEvalState[ offset ] = oldEvalState[ offset ];
			}
			for( ; offset < mSize ; ++offset )
			{
				mEvalState[ offset ] = PROCESS_UNDEFINED_STATE;
			}
		}

		delete [] oldEvalState;
	}

	else
	{
		mSize = aSize;

		allocTableOfState();
	}
}


/**
 * ALLOCATE - GETTER - SETTER
 * mTableOfAssignedFlag
 */

void TableOfRuntimeFormState::setAssigned(const ExecutionData & anED,
		avm_size_t rid, avm_size_t offset, bool flag)
{
	if( mTableOfAssignedFlag == NULL )
	{
		allocAssignedFlag(rid,
				anED.getRuntime(rid).getVariables().size(), false);
	}
	else if( mTableOfAssignedFlag[rid] == NULL )
	{
		mTableOfAssignedFlag[rid] = new Bitset(
				anED.getRuntime(rid).getVariables().size(), false );
	}

	( *(mTableOfAssignedFlag[rid]) )[offset] = flag;
}


void TableOfRuntimeFormState::setAssignedUnion(avm_size_t rid,
		Bitset * assignedTableA, Bitset * assignedTableB)
{
	if( assignedTableA != NULL )
	{
		if( mTableOfAssignedFlag == NULL )
		{
			reallocAssignedFlag();
		}

		mTableOfAssignedFlag[rid] = new Bitset( *assignedTableA );

		if( assignedTableB != NULL )
		{
			( *(mTableOfAssignedFlag[rid]) ) |= (*assignedTableB);
		}
	}
	else if( assignedTableB != NULL )
	{
		if( mTableOfAssignedFlag == NULL )
		{
			reallocAssignedFlag();
		}

		mTableOfAssignedFlag[rid] = new Bitset( *assignedTableB );
	}
}




/**
 * COMPARISON
 */
bool TableOfRuntimeFormState::equalsState(TableOfRuntimeFormState * other) const
{
	if( this->mEvalState != other->mEvalState )
	{
		if( this->size() == other->size() )
		{
			for( avm_size_t i = 0 ; i != this->size() ; ++i )
			{
				if( isNEQ(this->stateAt(i), other ->stateAt(i)) )
				{
					return( false );
				}
			}

			return( true );
		}
		else if( this->size() < other->size() )
		{
			avm_size_t i = 0;

			for( ; i != this->size() ; ++i )
			{
				if( this->stateAt(i) != other ->stateAt(i) )
				{
					return( false );
				}
			}

			for( ; i != other->size() ; ++i )
			{
				if( other->stateAt(i) != PROCESS_DESTROYED_STATE )
				{
					return( false );
				}
			}

			return( true );
		}
		else if( this->size() > other->size() )
		{
			avm_size_t i = 0;

			for( ; i != this->size() ; ++i )
			{
				if( this->stateAt(i) != other ->stateAt(i) )
				{
					return( false );
				}
			}

			for( ; i != this->size() ; ++i )
			{
				if( this->stateAt(i) != PROCESS_DESTROYED_STATE )
				{
					return( false );
				}
			}

			return( true );
		}
		else
		{
			return( false );
		}
	}

	return( true );
}



/**
 * Serialization
 */
void TableOfRuntimeFormState::toStream(OutStream & os) const
{
	for( avm_size_t offset = 0 ; offset != mSize ; ++offset )
	{
		os << TAB << "rid#" << offset << " = "
				<< RuntimeDef::strPES( mEvalState[offset] )
				<< ";" << EOL_FLUSH;
	}
}


void TableOfRuntimeFormState::toStream(
		const ExecutionData & anED, OutStream & os) const
{
	for( avm_size_t offset = 0 ; offset != mSize ; ++offset )
	{
//AVM_IF_DEBUG_LEVEL_GT_MEDIUM_OR( mEvalState[i] != PROCESS_IDLE_STATE )
		os << TAB << "<@rid#" << offset << " = "
				<< RuntimeDef::strPES( mEvalState[offset] ) << ";\t\t// "
				<< anED.getRuntime(offset).getFullyQualifiedNameID()
				<< EOL_FLUSH;
//AVM_IF_DEBUG_LEVEL_GT_MEDIUM_OR
	}
}

}
