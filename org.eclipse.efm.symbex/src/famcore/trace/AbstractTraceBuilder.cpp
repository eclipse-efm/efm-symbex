/*******************************************************************************
 * Copyright (c) 2016 CEA LIST.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Created on: 6 janv. 2014
 *
 * Contributors:
 *  Arnault Lapitre (CEA LIST) arnault.lapitre@cea.fr
 *   - Initial API and implementation
 ******************************************************************************/

#include "AbstractTraceBuilder.h"

#include <fml/workflow/Query.h>
#include <fml/workflow/WObject.h>


namespace sep
{

////////////////////////////////////////////////////////////////////////////
// CONFIGURE API
////////////////////////////////////////////////////////////////////////////

/*
prototype process::trace_generator as &avm::processor.TRACE_GENERATOR is
 section PROPERTY
  ...
  @data#selection = 'ALL';	// ALL | MODIFIED
  ...
 endsection PROPERTY

 ...

endprototype
*/

bool AbstractTraceBuilder::configure(const WObject * wfParameterObject)
{
	const WObject * thePROPERTY = Query::getRegexWSequence(
			wfParameterObject, OR_WID2("property", "PROPERTY"));

	if( thePROPERTY != WObject::_NULL_ )
	{
		mDataSelectionModifiedFlags = (Query::getRegexWPropertyString(
			thePROPERTY, CONS_WID2("data", "selection"), "") == "MODIFIED");

		oneTracePerfile = Query::getRegexWPropertyBoolean(thePROPERTY,
				CONS_WID4("one", "trace", "per", "file"), false);

		pathCountLimit = Query::getRegexWPropertySizeT(thePROPERTY,
				CONS_WID2("trace", "count"),
				(oneTracePerfile ? DEFAULT_TRACE_COUNT_LIMIT : AVM_NUMERIC_MAX_SIZE_T),
				AVM_NUMERIC_MAX_SIZE_T);
	}
	else
	{
		mDataSelectionModifiedFlags = true;
		oneTracePerfile = false;
		pathCountLimit = AVM_NUMERIC_MAX_SIZE_T;
	}

	return( configureImpl(wfParameterObject) );
}



} /* namespace sep */
