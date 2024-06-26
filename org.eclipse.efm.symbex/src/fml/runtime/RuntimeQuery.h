/*******************************************************************************
 * Copyright (c) 2016 CEA LIST.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Created on: 19 mars 2014
 *
 * Contributors:
 *  Arnault Lapitre (CEA LIST) arnault.lapitre@cea.fr
 *   - Initial API and implementation
 ******************************************************************************/

#ifndef FML_RUNTIME_RUNTIMEQUERY_H_
#define FML_RUNTIME_RUNTIMEQUERY_H_

#include <common/BF.h>

#include <fml/runtime/ExecutionData.h>


namespace sep
{

class Configuration;

class ExecutionData;

class ObjectElement;

//class UniFormIdentifier;

class WObject;


class RuntimeQuery
{

protected :
	/**
	 * ATTRIBUTES
	 */
	const Configuration & mConfiguration;

public:
	/**
	 * CONSTRUCTOR
	 * Default
	 */
	RuntimeQuery(const Configuration & aConfiguration)
	: mConfiguration( aConfiguration )
	{
		//!! NOTHING
	}


	////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////
	// STATIC TOOLS
	// Used during configuration step
	////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////

	/**
	 * SEARCH
	 * Symbol
	 * !UNUSED!
	 *
	const BF & searchSymbol(const WObject * aWProperty);

	std::size_t searchSymbol(const WObject * aWProperty, BFList & listofSymbol);
	*
	* !UNUSED!
	*/

	////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////
	// RUNTIME TOOLS
	// Used during execution< processing / filtering > step
	////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////

	/**
	 * SEARCH
	 * Variable
	 */

	BF searchVariable(const ExecutionData & anED, const RuntimeID & ctxRID,
			const std::string & aFullyQualifiedNameID) const ;

	inline BF searchVariable(const ExecutionData & anED,
			const std::string & aFullyQualifiedNameID) const
	{
		return( searchVariable(anED, anED.getRID(), aFullyQualifiedNameID) );
	}

//!@?UNUSED:
	const BF & searchVariable(const ExecutionData & anED,
			const RuntimeID & ctxRID, const ObjectElement & astElement) const;

	inline const BF & searchVariable(
			const ExecutionData & anED, const ObjectElement & astElement) const
	{
		return( searchVariable(anED, anED.getRID(), astElement) );
	}


	BF searchVariable(const ExecutionData & anED,
			const RuntimeID & ctxRID, const BF & aSymbolicParameter) const;

	inline BF searchVariable(
			const ExecutionData & anED, const BF & aSymbolicParameter) const
	{
		return( searchVariable(anED, anED.getRID(), aSymbolicParameter) );
	}


	/**
	 * SEARCH
	 * Symbol
	 * !UNUSED!
	 *
	BF searchSymbol(TableOfSymbol & aliasTable,
			const ExecutionData & anED, UniFormIdentifier * anUFI);

	const BF & searchSymbol(TableOfSymbol & aliasTable,
			const ExecutionData & anED, const ObjectElement & astElement);

	const BF & searchSymbol(TableOfSymbol & aliasTable,
			const ExecutionData & anED, const BF & aBaseInstance);
	*
	* !UNUSED!
	*/


	////////////////////////////////////////////////////////////////////////////
	// LIFELINE API
	////////////////////////////////////////////////////////////////////////////

	void getSystemLifelines(Vector< RuntimeID > & lifelines) const;

	const RuntimeID & getRuntimeByQualifiedNameID(
			const std::string & aQualifiedNameID) const;

};

} /* namespace sep */

#endif /* FML_RUNTIME_RUNTIMEQUERY_H_ */
